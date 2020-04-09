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
import com.amihaiemil.docker.Logs;
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
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.Collections;
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

	private static final Logger LOG = LoggerFactory.getLogger( DockerManager.class );
	public static final int DEFAULT_MAX_RUNNING_CONTAINERS = 3;

	final Docker docker;
	private Image image;
	private int maxcontainers = DEFAULT_MAX_RUNNING_CONTAINERS;
	private ExecutorService executor = Executors.newFixedThreadPool( maxcontainers );
	private ScheduledExecutorService threadreaper;
	private final Map<WorkItem, FutureTask<WorkItem>> runningThreads = Collections.synchronizedMap( new HashMap<>() );
	private ScheduledFuture<?> reaperop;

	private DockerManager( Docker d ) {
		docker = d;
	}

	public static DockerManager connect() {
            if( SystemUtils.IS_OS_WINDOWS ){
                try{
                    return new DockerManager( new TcpDocker( new URI( "http://localhost:2375" ) ) );
                }
                catch( URISyntaxException x ){
                    LOG.error( "invalid docker URI {}", x);
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
		if ( null != threadreaper ) {
			threadreaper.shutdownNow();
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

		// stop all our timer threads so the system can shutdown cleanly
		//runningThreads.values().forEach( ft -> ft.cancel( true ) );
	}

	private void startReaping() {
		if ( true ) {
			return;
		}

		if ( null == threadreaper ) {
			LOG.debug( "starting the conversion reaper" );
			threadreaper = Executors.newSingleThreadScheduledExecutor();
			// start a thread to stop threads that run too long
			reaperop = threadreaper.scheduleAtFixedRate( () -> {
				// go through our running WorkItems, and calculate running time
				LOG.debug( "conversion reaper approaches... {} {}", reaperop.isDone(), reaperop.isCancelled() );
				final LocalDateTime NOW = LocalDateTime.now();
				final int MAXDURR = App.prefs.getInt( Preference.CONVERSIONLIMIT, Integer.MAX_VALUE );

				if ( runningThreads.isEmpty() ) {
					LOG.debug( "nothing for reaper to reap" );
				}

				runningThreads.forEach( ( wi, ft ) -> {
					Duration timed = Duration.between( wi.getStarted(), NOW );
					LOG.debug( "checking item: {} ...runtime: {}s (max:{}s)", wi, timed.toSeconds(), MAXDURR );
					if ( Status.RUNNING == wi.getStatus() && MAXDURR <= timed.toSeconds() ) {
						ft.cancel( true );
					}
				} );
			}, 0, 5, TimeUnit.SECONDS );
		}
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
		startReaping();

		for ( WorkItem item : work ) {
			item.queued();
			l.itemChanged( item );

			FutureTask<WorkItem> conversion = new FutureTask<WorkItem>( () -> {
				try {
					// if the extension is stp, but the type is stpxml, we need to do
					// the STPtoXML conversion before we can convert (and remove XML afterwards)
					if ( needsStpToXmlConversion( item ) ) {
						Process proc = StpToXml.convert( item.getPath(), xmlPathForStp( item ) );
						int ret = proc.waitFor();
						if ( 0 != ret ) {
							item.error( "stp conversion failed" );
							l.itemChanged( item );
							return;
						}
					}

					Container c = createContainer( item );
					item.started( c.containerId() );
					l.itemChanged( item );
					c.start();
					LOG.debug( "container has been started for {}", item );
					int retcode = c.waitOn( null );
					Logs logs = c.logs();
					String stdout = logs.stdout().fetch();
					String stderr = logs.stderr().fetch();
					if ( retcode != 0 ) {
						item.error( "unknown error" );
					}
					else {
						item.finished( LocalDateTime.now() );
						c.remove();
					}
					l.itemChanged( item );

					LOG.debug( "done waiting! retcode: {}", retcode );
				}
				catch ( InterruptedException x ) {
					LOG.error( "Hey, I've been interrupted! {}", x );
					item.error( x.getMessage() );
					l.itemChanged( item );
				}
				catch ( IOException x ) {
					LOG.error( "{}", x );
					item.error( x.getMessage() );
					l.itemChanged( item );
				}
				finally {
					if ( needsStpToXmlConversion( item ) ) {
						xmlPathForStp( item ).toFile().delete();
					}
				}
			}, item ) {
				@Override
				public boolean cancel( boolean mayInterruptIfRunning ) {
					if ( super.cancel( mayInterruptIfRunning ) ) {
						for ( Container c : docker.containers() ) {
							if ( c.containerId().equals( item.getContainerId() ) ) {
								try {
									c.kill();
									LOG.debug( "cancelling (killing) item:{}", item );
									item.killed();
									l.itemChanged( item );
								}
								catch ( IOException x ) {
									LOG.error( "could not stop container for item: {}", item );
								}
							}
						}
					}
					return false;
				}
			};

			executor.submit( () -> {
				runningThreads.put( item, conversion );
				conversion.run();
				runningThreads.remove( item );
				if ( runningThreads.isEmpty() && null != threadreaper ) {
					LOG.debug( "stopping the conversion reaper (nothing to reap)" );
					threadreaper.shutdownNow();
					threadreaper = null;
				}
			} );
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
				.filter( wi -> Status.RUNNING == wi.getStatus() )
				.forEach( wi -> {
					JsonObject obj = statusmap.get( wi.getContainerId() );
					if ( null == obj ) {
						// we don't have this container anymore, so we can't tell what happened.
						// we need to re-convert this item
						wi.reinit();
					}
					else {
						switch ( obj.getString( "Status" ) ) {
							case "exited": {
								int retval = obj.getInt( "ExitCode" );
								if ( 0 == retval ) {
									String endtime = obj.getString( "FinishedAt" );
									try {
										ZonedDateTime zdt = ZonedDateTime.parse( endtime );
										wi.finished( zdt.toLocalDateTime() );
										containermap.get( wi.getContainerId() ).remove();
									}
									catch ( UnexpectedResponseException | IOException x ) {
										LOG.warn( "{}", x );
									}
								}
								else {
									wi.error( "unknown error at restart" );
								}
							}
							break;
							case "dead":
								wi.reinit();
								break;
							case "running":
								// if the item is still running, we need to listen for when
								// it's done, and update our table appropriately
								executor.submit( () -> {
									try {
										int retcode = containermap.get( wi.getContainerId() ).waitOn( null );
										if ( retcode != 0 ) {
											wi.error( "unknown error" );
										}
										else {
											wi.finished( LocalDateTime.now() );
											containermap.get( wi.getContainerId() ).remove();
										}
										l.itemChanged( wi );
									}
									catch ( IOException x ) {
										LOG.error( "{}", x );
									}
								} );
								break;
							default:
							// nothing to do here
						}
					}
				} );
	}
}
