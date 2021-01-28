/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.Conversion;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import org.apache.commons.io.FilenameUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class FormatConverter {

  private static final Logger LOG = LoggerFactory.getLogger( FormatConverter.class );

  public static ProcessInfo convert( Conversion src, Path inputfile ) throws IOException {
    File fmtcnv = initPath();

    if( null == inputfile ){
      inputfile = src.getItem().getPath();
    }

    List<String> cmds = List.of( fmtcnv.toString(),
          inputfile.toString(),
          "--to", "hdf5",
          "--localtime",
          "--pattern", src.getItem().getOutputPath().resolve( "%S" ).toString()
    );
    if ( !Objects.isNull( src.getItem().getType() ) ) {
      cmds = new ArrayList<>( cmds );
      cmds.addAll( List.of( "--from", src.getItem().getType() ) );
    }

    LOG.debug( "FormatConverter command: {}", String.join( " ", cmds ) );

    File stdout = src.getLogDir().resolve( "conv-output.log" ).toFile();
    File stderr = src.getLogDir().resolve( "conv-errors.log" ).toFile();
    Process proc = new ProcessBuilder()
          .command( cmds )
          .redirectError( stderr )
          .redirectOutput( stdout )
          .directory( fmtcnv.getParentFile() )
          .start();

    return new ProcessInfo( proc, fmtcnv.getParentFile(), stdout, stderr );
  }

  /**
   * Initializes the STPtoXML program, and copies it to a new location. STPtoXML
   * seems to struggle when it's run in multiple threads from the same directory
   *
   * @return the new location
   * @throws IOException
   */
  private static File initPath() throws IOException {
    Path tmpdir = Files.createTempDirectory( "prevent." ).resolve( "fmtcnv" );
    File extractiondir = tmpdir.toFile();

    LOG.info( "extracting format converter to: {}", extractiondir );
    extractiondir.mkdirs();

    String srcjar = ( SystemUtils.IS_OS_WINDOWS
                      ? "/formatconverter-win.jar"
                      : "/formatconverter-linux.jar" );

    File exe = null;
    try ( JarInputStream jis = new JarInputStream( FormatConverter.class.getResourceAsStream( srcjar ) ) ) {
      JarEntry je;
      while ( ( je = jis.getNextJarEntry() ) != null ) {
        if ( !je.isDirectory() ) {
          File f = new File( extractiondir, FilenameUtils.getName( je.getName() ) );
          try ( OutputStream os = new FileOutputStream( f ) ) {
            IOUtils.copy( jis, os );
            if ( f.getName().startsWith( "formatconverter" ) ) {
              exe = f;
              exe.setExecutable( true );
            }
          }
        }
      }
    }

    return exe;
  }
}
