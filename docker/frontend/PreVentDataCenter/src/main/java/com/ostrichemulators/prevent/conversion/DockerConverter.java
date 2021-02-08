/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.amihaiemil.docker.Container;
import com.amihaiemil.docker.Docker;
import com.amihaiemil.docker.Image;
import com.amihaiemil.docker.Images;
import com.amihaiemil.docker.TcpDocker;
import com.amihaiemil.docker.UnexpectedResponseException;
import com.amihaiemil.docker.UnixDocker;
import com.ostrichemulators.prevent.App;
import com.ostrichemulators.prevent.Conversion;
import com.ostrichemulators.prevent.WorkItem;
import com.ostrichemulators.prevent.WorkItem.Status;
import com.ostrichemulators.prevent.WorkItemStateChangeListener;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.FutureTask;
import java.util.zip.GZIPOutputStream;
import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class DockerConverter extends AbstractConverter {

  private static final Logger LOG = LoggerFactory.getLogger( DockerConverter.class );
  private static Set<WorkItem.Status> REINIT_STATUSES = Set.of( WorkItem.Status.QUEUED, WorkItem.Status.PREPROCESSING );
  private Docker docker;
  private Image image;

  public DockerConverter() {
  }

  @Override
  public void verifyAndPrepare() {
    try {
      docker = ( SystemUtils.IS_OS_WINDOWS
                 ? new TcpDocker( new URI( "http://localhost:2375" ) )
                 : new UnixDocker( new File( "/var/run/docker.sock" ) ) );

      if ( docker.ping() ) {
        image = null;

        Map<String, Iterable<String>> filters = new HashMap<>();
        filters.put( "reference", List.of( "ry99/prevent" ) );

        Images images = docker.images().filter( filters );
        if ( images.iterator().hasNext() ) {
          image = images.iterator().next();
        }
        else {
          LOG.debug( "PreVent image not found" );
          image = images.pull( "ry99/prevent", "latest" );
        }

        LOG.debug( "pulling  ry99/prevent:latest" );
        image = images.pull( "ry99/prevent", "latest" );
      }
    }
    catch ( URISyntaxException | IOException x ) {
      throw new RuntimeException( "Is the Docker service running?", x );
    }
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

  private Container createContainer( Conversion item ) throws IOException {
    LOG.debug( "creating container from image: ", image.toString() );
    JsonArrayBuilder mounts = Json.createArrayBuilder();
    JsonObjectBuilder volumes = Json.createObjectBuilder();
    mounts.add( String.format( "%s:/opt/todo", item.getItem().getPath().getParent().toString() ) );
    mounts.add( String.format( "%s:/opt/output", item.getItem().getOutputPath().toString() ) );

    volumes.add( "/opt/todo", Json.createObjectBuilder() );
    volumes.add( "/opt/output", Json.createObjectBuilder() );

    JsonObjectBuilder hostconfig = Json.createObjectBuilder();
    hostconfig.add( "Binds", mounts );

    boolean doxml = item.needsStpToXml();
    JsonArrayBuilder cmds = Json.createArrayBuilder( List.of( "--to", "hdf5",
          "--pattern", "/opt/output/%S",
          "--localtime",
          String.format( "/opt/todo/%s", ( doxml
                                           ? item.getXmlPath()
                                           : item.getItem().getPath() ).getFileName() ) ) );
    if ( null != item.getItem().getType() ) {
      cmds.add( "--from" ).add( item.getItem().getType() );
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

  public void reinitialize( Conversion item, WorkItemStateChangeListener l ) throws IOException {
    Map<String, JsonObject> statusmap = new HashMap<>();
    Map<String, Container> containermap = new HashMap<>();
    getContainerStates().forEach( ( c, o ) -> {
      statusmap.put( c.containerId(), o );
      containermap.put( c.containerId(), c );
    } );

    // anything that was queued on the last run should be reset since
    // that executor is no longer running
    if ( REINIT_STATUSES.contains( item.getItem().getStatus() ) ) {
      item.getItem().reinit();
    }

    if ( item.isStatus( Status.RUNNING ) ) {
      JsonObject obj = statusmap.get( item.getItem().getContainerId() );
      if ( null == obj ) {
        // we don't have this container anymore, so we can't tell what
        // happened. we need to re-convert this item
        item.getItem().error( "missing container" );
      }
      else {
        switch ( obj.getString( "Status" ) ) {
          case "exited": {
            int retval = obj.getInt( "ExitCode" );

            try {
              saveContainerLogs( item, containermap.get( item.getItem().getContainerId() ) );
            }
            catch ( IOException x ) {
              LOG.error( "unable to save logs: {}", item );
            }

            if ( 0 == retval ) {
              String endtime = obj.getString( "FinishedAt" );
              try {
                ZonedDateTime zdt = ZonedDateTime.parse( endtime );
                LOG.debug( "setting finished time from container for item:{}", item );
                item.getItem().finished( zdt.withZoneSameInstant( ZoneId.systemDefault() ).toLocalDateTime() );
                if ( App.prefs.removeDockerOnSuccess() ) {
                  containermap.get( item.getItem().getContainerId() ).remove();
                }
              }
              catch ( UnexpectedResponseException | IOException x ) {
                LOG.warn( "{}", x );
              }
            }
            else {
              item.getItem().error( "unknown error at restart" );
            }
          }
          break;
          case "dead":
            item.getItem().reinit();
            break;
          case "running":
            // if the item is still running, we need to listen for when
            // it's done, and update our table appropriately
            LOG.debug( "attaching to running container for item: {}", item );
            //executor.submit( new ConversionTask( item, l, containermap.get( item.getContainerId() ) ) );
            break;
          default:
          // nothing to do here
        }
        item.tellListeners();
      }
    }
  }

  @Override
  public FutureTask<WorkItem> createTask( Conversion item, Object monitor ) {
    return new FutureTask<>( () -> {
      ProcessInfo cnv = null;
      try {
        File f = item.getLogDir().toFile();
        if ( !f.exists() ) {
          f.mkdirs();
        }

        if ( item.needsStpToXml() ) {
          item.getItem().preprocess();
          item.tellListeners();

          cnv = StpToXml.convert( item );
          synchronized ( monitor ) {
            while ( cnv.process.isAlive() && StopReason.DONT_STOP == conversionCanWaitLonger( item ) ) {
              try {
                monitor.wait();
              }
              catch ( InterruptedException ie ) {
                if ( isShuttingDown() ) {
                  cnv.process.destroyForcibly();
                }
                else {
                  LOG.warn( "ignoring this interruption (preprocessing): {}", ie.getLocalizedMessage() );
                }
              }
            }
          }

          int ret = cnv.process.exitValue();
          if ( 0 != ret ) {
            // FIXME: save the output log somewhere
            item.getItem().error( "stp conversion failed" );
            item.tellListeners();
            return;
          }
        }

        Container c = createContainer( item );
        item.getItem().started( c.containerId() );
        item.tellListeners();
        c.start();
        LOG.debug( "container has been started for {}", item );

        synchronized ( monitor ) {
          StopReason reason = StopReason.DONT_STOP;
          while ( StopReason.DONT_STOP == reason ) {
            try {
              monitor.wait();
              reason = this.conversionCanWaitLonger( item );
            }
            catch ( InterruptedException ue ) {
              if ( isShuttingDown() ) {
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

          // FIXME: no matter how we got to this point, save the container logs somewhere
          saveContainerLogs( item, c );

          switch ( reason ) {
            case TOO_LONG:
              item.getItem().killed();
              break;
            case ERROR:
              item.getItem().error( "container error" );
              break;
            case COMPLETED:
              item.getItem().finished( LocalDateTime.now() );
              if ( App.prefs.removeDockerOnSuccess() ) {
                c.remove(); // remove containers that completed successfully
              }
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
      catch ( IOException x ) {
        LOG.error( "{}", x );
        item.getItem().error( x.getMessage() );
      }

      finally {
        item.tellListeners();

        if ( item.needsStpToXml() ) {
          LOG.debug( "removing XML from STPtoXML conversion: {}", item.getXmlPath() );
          item.getXmlPath().toFile().delete();
        }
      }
    }, item.getItem() );
  }

  private void saveContainerLogs( Conversion item, Container container ) throws IOException {
    File datadir = App.getDataDirLocation().resolve( item.getItem().getId() ).toFile();
    boolean ok = ( datadir.exists() && datadir.isDirectory()
                   ? true
                   : datadir.mkdirs() );
    if ( !ok ) {
      throw new RuntimeException( "unable to save logs: " + datadir );
    }

    try ( OutputStream outstream = new GZIPOutputStream( new FileOutputStream( new File( datadir, "stderr.gz" ) ) ) ) {
      String log = container.logs().stderr().fetch();
      IOUtils.write( log, outstream, StandardCharsets.UTF_8.toString() );
    }

    try ( OutputStream outstream = new GZIPOutputStream( new FileOutputStream( new File( datadir, "stdout.gz" ) ) ) ) {
      String log = container.logs().stdout().fetch();
      IOUtils.write( log, outstream, StandardCharsets.UTF_8.toString() );
    }
  }

  @Override
  public Conversion reinitialize( Conversion item ) {
    item.getItem().reinit();
    return item;
  }
}
