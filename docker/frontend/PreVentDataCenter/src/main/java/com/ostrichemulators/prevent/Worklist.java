/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class Worklist {

	private static final Logger LOG = LoggerFactory.getLogger( Worklist.class );

	private static Worklist list;
	private final List<WorkItem> items = new ArrayList<>();
	private final Path saveloc;
	private static final ObjectMapper objmap = new ObjectMapper();

	private Worklist( Path saver ) {
		saveloc = saver;
		File file = saveloc.toFile();
		if ( file.exists() ) {
			try {
				WorkItem[] reads = objmap.readValue( saveloc.toFile(), WorkItem[].class );
				items.addAll( Arrays.asList( reads ) );
			}
			catch ( IOException x ) {
				throw new RuntimeException( x );
			}
		}
	}

	public static Worklist open( Path saveloc ) {
		if ( null == list ) {
			objmap.registerModule( new JavaTimeModule() );
			objmap.configure( SerializationFeature.WRITE_DATES_AS_TIMESTAMPS, false );
			list = new Worklist( saveloc );
		}
		return list;
	}

	public List<WorkItem> getItems() {
		return Collections.unmodifiableList( items );
	}

	public boolean add( WorkItem i ) {
		if ( items.add( i ) ) {
			try {
				save();
				return true;
			}
			catch ( IOException x ) {
				items.remove( i );
				LOG.error( "{}", x );
			}
		}
		return false;
	}

	public void update( WorkItem i ) {
		LOG.warn( "not yet implemented" );
		try {
			save();
		}
		catch ( IOException x ) {
			LOG.error( "{}", x );
		}
	}

	private void save() throws IOException {
		objmap.writeValue( saveloc.toFile(), items );
	}
}
