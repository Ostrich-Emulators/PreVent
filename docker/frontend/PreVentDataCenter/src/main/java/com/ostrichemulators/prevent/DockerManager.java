/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.amihaiemil.docker.Container;
import com.amihaiemil.docker.Containers;
import com.amihaiemil.docker.Docker;
import com.amihaiemil.docker.Image;
import com.amihaiemil.docker.Images;
import com.amihaiemil.docker.LocalDocker;
import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class DockerManager {

	private static final Logger LOG = LoggerFactory.getLogger( DockerManager.class );

	final Docker docker;
	private Image image;
	private final List<Container> runningContainers = new ArrayList<>();

	private DockerManager( Docker d ) {
		docker = d;
	}

	public static DockerManager connect() {
		return new DockerManager( SystemUtils.IS_OS_WINDOWS
				? new LocalDocker( new File( "/var/run/docker.sock" ) ) // don't know what the Windows equivalent is yet
				: new LocalDocker( new File( "/var/run/docker.sock" ) ) );
	}

	public boolean verifyOrPrepare() {
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

				runningContainers.clear();
				getContainerStates().forEach( ( c, s ) -> {
					if ( "running".equals( s ) ) {
						runningContainers.add( c );
					}
				} );
			}
		}
		catch ( IOException x ) {
			LOG.error( "Is the Docker service running?", x );
		}
		return ( null != image );
	}

	private Map<Container, String> getContainerStates() {
		Map<Container, String> containers = new HashMap<>();

		Map<String, Iterable<String>> f = new HashMap<>();
		f.put( "ancestor", List.of( "ry99/prevent" ) );
		//f.put( "status", List.of( "running" ) );
		docker.containers().all().forEachRemaining( c -> {
			try {
				JsonObject data = c.inspect().getJsonObject("State");
				String status = data.getString( "Status" );
				containers.put( c, status );
			}
			catch ( IOException x ) {
				LOG.warn( "Unable to inspect container: {}", c.containerId(), x );
			}
		} );

		return containers;
	}

	public String run( Collection<WorkItem> work ) throws IOException {
		Map<String, Path> neededDirs = new HashMap<>();

		work.stream()
				.map( wi -> wi.getFile().getParent() )
				.distinct()
				.forEach( path -> neededDirs.put( String.format( "/opt/converter-%d", neededDirs.size() ), path ) );

		JsonArrayBuilder mounts = Json.createArrayBuilder();
		JsonObjectBuilder volumes = Json.createObjectBuilder();
		neededDirs.forEach( ( mount, path ) -> {
			volumes.add( mount, Json.createObjectBuilder() );
			mounts.add( String.format( "%s:%s", path, mount ) );
		} );

		JsonObjectBuilder hostconfig = Json.createObjectBuilder();
		hostconfig.add( "Binds", mounts );

		Containers containers = docker.containers();
		JsonObjectBuilder config = Json.createObjectBuilder();

		config.add( "Image", "ry99/prevent" );
		//config.add( "Volumes", volumes );
		config.add( "HostConfig", hostconfig );

		JsonObject json = config.build();
		LOG.debug( "{}", json );

		Container cc = containers.create( json );

		// make a new thread to feed this container all the files from work
		return cc.containerId();
	}
}
