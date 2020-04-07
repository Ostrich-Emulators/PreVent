/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import org.apache.commons.io.FilenameUtils;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class StpToXml {

	private static final Logger LOG = LoggerFactory.getLogger( StpToXml.class );
	private static Path stpdir;

	public static void convert( Path stpfile, Path xmlfile ) throws IOException {
		initIfNeeded();
	}

	private static void initIfNeeded() throws IOException {
		if ( null == stpdir ) {
			String tmpdir = App.prefs.get( "tools.parser.stptoxml", "missing" );
			if ( "missing".equals( tmpdir ) ) {
				tmpdir = Files.createTempDirectory( "prevent." ).toString();
				App.prefs.put( "tools.parser.stptoxml", tmpdir );
			}
			File extractiondir = new File( tmpdir, "StpToolkit_8.2" );
			stpdir = extractiondir.toPath();
			if ( !( new File( extractiondir, "StpToolkit.exe" ).exists() ) ) {
				extractiondir.mkdirs();

				try ( JarInputStream jis = new JarInputStream( StpToXml.class.getResourceAsStream( "/stptoxml.jar" ) ) ) {
					JarEntry je;
					while ( ( je = jis.getNextJarEntry() ) != null ) {
						if ( !je.isDirectory() ) {
							File f = new File( extractiondir, FilenameUtils.getName( je.getName() ) );
							try ( OutputStream os = new FileOutputStream( f ) ) {
								IOUtils.copy( jis, os );
							}
						}
					}
				}
			}
		}
	}
}
