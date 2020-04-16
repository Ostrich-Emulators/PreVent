/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.amihaiemil.docker.Container;
import com.amihaiemil.docker.Docker;
import com.amihaiemil.docker.Image;
import com.amihaiemil.docker.Images;
import com.amihaiemil.docker.TcpDocker;
import com.amihaiemil.docker.UnexpectedResponseException;
import com.amihaiemil.docker.UnixDocker;
import com.ostrichemulators.prevent.WorkItem.Status;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;
import org.apache.commons.io.FilenameUtils;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class DockerManager {

  public static final int DEFAULT_MAX_RUNNING_CONTAINERS = 3;
  private static final Logger LOG = LoggerFactory.getLogger( DockerManager.class );
  private static final int CHECK_INTERVAL_S = 5;

  private static enum StopReason {
    TOO_LONG, ERROR, SHUTDOWN, COMPLETED, DONT_STOP
  };

  final Docker docker;
  private Image image;
  private int maxcontainers = DEFAULT_MAX_RUNNING_CONTAINERS;
  private ExecutorService executor = Executors.newFixedThreadPool( maxcontainers );
  private ScheduledExecutorService threadreaper;
  private ScheduledFuture<?> reaperop;
  private boolean shuttingDown = false;
  private final Object monitor = new Object();

  private DockerManager( Docker d ) {
    docker = d;
  }

  public static DockerManager connect() {
    if ( SystemUtils.IS_OS_WINDOWS ) {
      try {
        return new DockerManager( new TcpDocker( new URI( "http://localhost:2375" ) ) );
      }
      catch ( URISyntaxException x ) {
        LOG.error( "invalid docker URI {}", x );
      }
    }

    return new DockerManager( new UnixDocker( new File( "/var/run/docker.sock" ) ) );
  }

  public void setMaxContainers( int maxx ) {
    if ( maxx != maxcontainers ) {
      executor.shutdown();
      try {
        if ( !executor.awaitTermination( 500, TimeUnit.MILLISECONDS ) ) {
          executor.shutdownNow();
        }
      }
      catch ( InterruptedException e ) {
        executor.shutdownNow();
      }

      maxcontainers = maxx;
      executor = Executors.newFixedThreadPool( maxcontainers );
    }
  }

  public int getMaxContainers() {
    return maxcontainers;
  }

  public boolean verifyAndPrepare() {
    try {
      if ( docker.ping() ) {
        image = null;

        Map<String, Iterable<String>> filters = new HashMap<>();
        filters.put( "reference", List.of( "ry99/prevent" ) );

        Images images = docker.images().filter( filters );
        if ( images.iterator().hasNext() ) {
          image = images.iterator().next();
        }
        else {
          LOG.debug( "PreVent image not found...fetching latest" );
          image = images.pull( "ry99/prevent", "latest" );
        }

        startReaping();
      }
    }
    catch ( IOException x ) {
      LOG.error( "Is the Docker service running?", x );
    }
    return ( null != image );
  }

  public boolean isRunning( Container c ) {
    JsonObject obj = getContainerStates().get( c );
    return ( null == obj
             ? false
             : "running".equals( obj.getString( "Status" ) ) );
  }

  public Map<Container, JsonObject> getContainerStates() {
    Map<Container, JsonObject> containers = new HashMap<>();

    Map<String, Iterable<String>> f = new HashMap<>();
    f.put( "ancestor", List.of( "ry99/prevent" ) );
    docker.containers().all().forEachRemaining( c -> {
      try {
        JsonObject data = c.inspect().getJsonObject( "State" );
        containers.put( c, data );
      }
      catch ( IOException x ) {
        LOG.warn( "Unable to inspect container: {}", c.containerId(), x );
      }
    } );

    return containers;
  }

  public void shutdown() {
    shuttingDown = true;

    LOG.info( "shutting down docker manager" );
    if ( null != threadreaper ) {
      threadreaper.shutdownNow();

      LOG.debug( "cancelling reaperop thread" );
      reaperop.cancel( true );
    }

    executor.shutdown();
    try {
      if ( !executor.awaitTermination( 5, TimeUnit.SECONDS ) ) {
        executor.shutdownNow();
      }
    }
    catch ( InterruptedException e ) {
      executor.shutdownNow();
    }

    // tell all our running threads to shutdown
    synchronized ( monitor ) {
      monitor.notifyAll();
    }
    LOG.debug( "done shutting down" );
  }

  private void startReaping() {
    // Our threadreaper just refreshes the container states every X seconds
    // and notifies the worker threads to see what happened. The worker threads
    // need to figure out for themselves if/how they should exit.

    LOG.debug( "starting the conversion reaper" );
    threadreaper = Executors.newSingleThreadScheduledExecutor();
    // start a thread to wake up threads every few seconds
    reaperop = threadreaper.scheduleAtFixedRate( () -> {
      synchronized ( monitor ) {
        monitor.notifyAll();
      }
    }, 0, CHECK_INTERVAL_S, TimeUnit.SECONDS );
  }

  private static Path xmlPathForStp( WorkItem item ) {
    File f = item.getPath().toFile();
    String filename = FilenameUtils.getBaseName( f.toString() );
    return Paths.get( f.getParent(), filename + ".xml" );
  }

  private static boolean needsStpToXmlConversion( WorkItem item ) {
    return ( "stp".equalsIgnoreCase( FilenameUtils.getExtension( item.getPath().toString() ) )
          && "stpxml".equalsIgnoreCase( item.getType() ) );

  }

  public void convert( Collection<WorkItem> work, WorkItemStateChangeListener l ) throws IOException {
    for ( WorkItem item : work ) {
      item.queued();
      l.itemChanged( item );
      executor.submit( new ConversionTask( item, l, null ) );
    }
  }

  private Container createContainer( WorkItem item ) throws IOException {
    JsonArrayBuilder mounts = Json.createArrayBuilder();
    JsonObjectBuilder volumes = Json.createObjectBuilder();
    mounts.add( String.format( "%s:/opt/todo", item.getPath().getParent().toString() ) );
    volumes.add( "/opt/todo", Json.createObjectBuilder() );

    JsonObjectBuilder hostconfig = Json.createObjectBuilder();
    hostconfig.add( "Binds", mounts );

    JsonArrayBuilder cmds = Json.createArrayBuilder( List.of(
          "--to",
          "hdf5",
          "--localtime",
          String.format( "/opt/todo/%s", ( needsStpToXmlConversion( item )
                                           ? xmlPathForStp( item )
                                           : item.getPath() ).getFileName() ) ) );
    if ( null != item.getType() ) {
      cmds.add( "--from" ).add( item.getType() );
    }

    JsonObjectBuilder config = Json.createObjectBuilder();
    config.add( "Image", "ry99/prevent" );
    //config.add( "Volumes", volumes );
    config.add( "HostConfig", hostconfig );
    config.add( "Cmd", cmds );

    JsonObject json = config.build();
    //LOG.debug( "{}", json );
    Container cc = docker.containers().create( json );
    return cc;
  }

  public void reinitializeItems( Collection<WorkItem> items, WorkItemStateChangeListener l ) throws IOException {
    Map<String, JsonObject> statusmap = new HashMap<>();
    Map<String, Container> containermap = new HashMap<>();
    getContainerStates().forEach( ( c, o ) -> {
      statusmap.put( c.containerId(), o );
      containermap.put( c.containerId(), c );
    } );

    // anything that was queued on the last run should be reset since
    // that executor is no longer running
    items.stream()
          .filter( wi -> Status.QUEUED == wi.getStatus() )
          .forEach( wi -> wi.reinit() );

    // we only need to update items that are in the running state, in case
    // they finished running while the app wasn't running
    items.stream()
          .filter( item -> Status.RUNNING == item.getStatus() )
          .forEach( item -> {
            JsonObject obj = statusmap.get( item.getContainerId() );
            if ( null == obj ) {
              // we don't have this container anymore, so we can't tell what
              // happened. we need to re-convert this item
              item.error( "missing container" );
            }
            else {
              switch ( obj.getString( "Status" ) ) {
                case "exited": {
                  int retval = obj.getInt( "ExitCode" );
                  if ( 0 == retval ) {
                    String endtime = obj.getString( "FinishedAt" );
                    try {
                      ZonedDateTime zdt = ZonedDateTime.parse( endtime );
                      LOG.debug( "setting finished time from container for item:{}", item );
                      item.finished( zdt.withZoneSameInstant( ZoneId.systemDefault() ).toLocalDateTime() );
                      containermap.get( item.getContainerId() ).remove();
                    }
                    catch ( UnexpectedResponseException | IOException x ) {
                      LOG.warn( "{}", x );
                    }
                  }
                  else {
                    item.error( "unknown error at restart" );
                  }
                }
                break;
                case "dead":
                  item.reinit();
                  break;
                case "running":
                  // if the item is still running, we need to listen for when
                  // it's done, and update our table appropriately
                  LOG.debug( "attaching to running container for item: {}", item );
                  executor.submit( new ConversionTask( item, l, containermap.get( item.getContainerId() ) ) );
                  break;
                default:
                // nothing to do here
              }
              l.itemChanged( item );
            }
          } );
  }

  private StopReason threadCanWaitLonger( WorkItem item, JsonObject state ) {

    // if we're shutting down, we can't continue
    if ( shuttingDown ) {
      return StopReason.SHUTDOWN;
    }

    // container doesn't exist? error
    if ( null == state ) {
      return StopReason.ERROR;
    }

    // check if we're running too long
    final LocalDateTime NOW = LocalDateTime.now();
    final int MAXDURR = App.prefs.getInt( Preference.CONVERSIONLIMIT, Integer.MAX_VALUE );

    Duration timed = Duration.between( item.getStarted(), NOW );
    //LOG.debug( "checking item: {} ...runtime: {}m (max:{}m)", item, timed.toMinutes(), MAXDURR );
    if ( Status.RUNNING == item.getStatus() && timed.toMinutes() >= MAXDURR ) {
      LOG.debug( "reaping work item {} (running too long)", item );
      return StopReason.TOO_LONG;
    }

    final String status = state.getString( "Status" );
    // it's okay if we're still running
    if ( "running".equalsIgnoreCase( status ) ) {
      return StopReason.DONT_STOP;
    }

    // if we exited, then we need to know why
    if ( "exited".equalsIgnoreCase( status ) ) {
      return ( 0 == state.getInt( "ExitCode" )
               ? StopReason.COMPLETED
               : StopReason.ERROR );
    }

    return StopReason.ERROR;
  }

  private class ConversionTask extends FutureTask<WorkItem> {

    ConversionTask( WorkItem item, WorkItemStateChangeListener listener, Container reinitContainer ) {
      super( () -> {
        try {
          Container c;
          // if the extension is stp, but the type is stpxml, we need to do
          // the STPtoXML conversion before we can convert (and remove XML afterwards)
          if ( null != reinitContainer ) {
            c = reinitContainer;
          }
          else {
            if ( needsStpToXmlConversion( item ) ) {
              Process proc = StpToXml.convert( item.getPath(), xmlPathForStp( item ) );
              int ret = proc.waitFor();
              if ( 0 != ret ) {
                item.error( "stp conversion failed" );
                listener.itemChanged( item );
                return;
              }
            }

            c = createContainer( item );
            item.started( c.containerId() );
            listener.itemChanged( item );
            c.start();
            LOG.debug( "container has been started for {}", item );
          }

          synchronized ( monitor ) {
            StopReason reason = StopReason.DONT_STOP;
            while ( StopReason.DONT_STOP == reason ) {
              try {
                DockerManager.this.monitor.wait();
                reason = DockerManager.this.threadCanWaitLonger( item, c.inspect().getJsonObject( "State" ) );
              }
              catch ( InterruptedException ue ) {
                if ( shuttingDown ) {
                  reason = StopReason.SHUTDOWN;
                }
                else {
                  LOG.warn( "ignoring this interruption: {}", ue.getLocalizedMessage() );
                }
              }
            }
            // okay, we can't wait any longer.
            // this means one of 4 things:
            // 1) the conversion has completed normally
            // 2) the conversion ended in error
            // 3) the conversion was killed (timed out)
            // 4) the system is shutting down
            switch ( reason ) {
              case TOO_LONG:
                item.killed();
                break;
              case ERROR:
                item.error( "container error" );
                break;
              case COMPLETED:
                item.finished( LocalDateTime.now() );
                c.remove(); // remove containers that completed successfully
                break;
              case SHUTDOWN:
                // don't do anything here (the container is still running)
                break;
              default:
                // we should never get here!
                throw new IllegalStateException( "BUG! Unhandled stop reason: " + reason );
            }
          }

          LOG.debug( "done waiting! " );
        }
        catch ( InterruptedException x ) {
          LOG.error( "Hey, I've been interrupted! {}", x );
          item.error( x.getMessage() );
        }
        catch ( IOException x ) {
          LOG.error( "{}", x );
          item.error( x.getMessage() );
        }
        finally {
          listener.itemChanged( item );

          if ( needsStpToXmlConversion( item ) ) {
            xmlPathForStp( item ).toFile().delete();
          }
        }
      }, item );
    }
  }
}
