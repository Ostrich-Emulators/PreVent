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
import java.util.List;

/**
 *
 * @author ryan
 */
public class Worklist {

	private static ObjectMapper objmap;

	public static List<WorkItem> open( Path saveloc ) throws IOException {
		List<WorkItem> list = new ArrayList<>();
		if ( null == objmap ) {
			objmap = new ObjectMapper();
			objmap.registerModule( new JavaTimeModule() );
			objmap.configure( SerializationFeature.WRITE_DATES_AS_TIMESTAMPS, false );
		}

		File file = saveloc.toFile();
		if ( file.exists() ) {
			WorkItem[] reads = objmap.readValue( saveloc.toFile(), WorkItem[].class );
			list.addAll( Arrays.asList( reads ) );
		}
		return list;
	}

	public static void save( List<WorkItem> items, Path savedloc ) throws IOException {
		objmap.writeValue( savedloc.toFile(), items );
	}
}
