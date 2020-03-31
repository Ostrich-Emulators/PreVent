/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
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
	private final ObjectMapper objmap = new ObjectMapper();

	private Worklist( Path saver ) {
		saveloc = saver;
		File file = saveloc.toFile();
		if ( file.exists() ) {
			try {
				objmap.readValue( saveloc.toFile(), List.class );
			}
			catch ( IOException x ) {
				throw new RuntimeException( x );
			}
		}
	}

	public static Worklist open( Path saveloc ) {
		if ( null == list ) {
			list = new Worklist( saveloc );
		}
		return list;
	}

	public List<WorkItem> getItems() {
		return Collections.unmodifiableList( items );
	}

	public boolean add( WorkItem i ) throws IOException {
		boolean added = items.add( i );
		save();
		return added;
	}

	public void update( WorkItem i ) throws IOException {
		LOG.warn( "not yet implemented" );
		save();
	}

	private void save() throws IOException {
		objmap.writeValue( saveloc.toFile(), items );
	}
}
