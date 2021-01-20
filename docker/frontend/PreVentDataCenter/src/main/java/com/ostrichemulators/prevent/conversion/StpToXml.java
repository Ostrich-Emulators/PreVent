/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.App;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import org.apache.commons.io.FileUtils;
import org.apache.commons.io.FilenameUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class StpToXml {

  private static final Logger LOG = LoggerFactory.getLogger( StpToXml.class );
  private static Path stpdir;

  public static ProcessInfo convert( Path stpfile, Path xmlfile ) throws IOException {
    initIfNeeded();

    // STPtoXML seems to struggle when multiple conversions are running from the
    // same directory, so we copy stpdir to a new dir for every conversion
    File dir = Files.createTempDirectory( "prevent-stptoxml." ).toFile();
    FileUtils.copyDirectory( stpdir.toFile(), dir );

    List<String> cmds = new ArrayList<>();
    if ( !SystemUtils.IS_OS_WINDOWS ) {
      cmds.add( "mono" );
    }
    cmds.add( new File( dir, "StpToolkit.exe" ).toString() );
    cmds.add( stpfile.toString() );
    cmds.add( "-o" );
    cmds.add( xmlfile.toString() );
    //cmds.add( "-utc" );
    if ( App.prefs.isStpPhilips() ) {
      cmds.add( "-p" );
    }
    LOG.debug( "STPtoXML command: {}", String.join( " ", cmds ) );

    File stdout = new File( dir, "stp-output.log" );
    File stderr = new File( dir, "stp-errors.log" );
    Process proc = new ProcessBuilder()
          .command( cmds )
          .redirectError( stderr )
          .redirectOutput( stdout )
          .directory( dir )
          .start();

    return new ProcessInfo( proc, dir, stdout, stderr );
  }

  /**
   * Initializes the STPtoXML program, and copies it to a new location. STPtoXML
   * seems to struggle when it's run in multiple threads from the same directory
   *
   * @return the new location
   * @throws IOException
   */
  private static void initIfNeeded() throws IOException {
    if ( null == stpdir ) {
      File tmpdir = App.prefs.getStpDir();
      if ( null == tmpdir ) {
        tmpdir = Files.createTempDirectory( "prevent." ).toFile();
        App.prefs.setStpDir( tmpdir.toPath() );
      }
      File extractiondir = new File( tmpdir, "StpToolkit_8.4" );
      stpdir = extractiondir.toPath();
      if ( !( new File( extractiondir, "StpToolkit.exe" ).exists() ) ) {
        LOG.info( "extracting STPtoXML to: {}", extractiondir );
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
