/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.Conversion;
import com.ostrichemulators.prevent.WorkItem.Status;
import com.ostrichemulators.prevent.conversion.Converter.StopReason;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public abstract class AbstractConverter implements Converter {

  private static final Logger LOG = LoggerFactory.getLogger( AbstractConverter.class );
  private boolean shuttingDown = false;

  protected static enum LogType {
    STP, CONVERSION
  };

  protected StopReason conversionCanWaitLonger( Conversion conv ) {

    // if we're shutting down, we can't continue
    if ( shuttingDown ) {
      return StopReason.SHUTDOWN;
    }

    boolean runTooLong = conv.runOverlong();
    // if we're preprocessing just check running time
    if ( conv.isStatus( Status.PREPROCESSING ) ) {
      return ( runTooLong
               ? StopReason.TOO_LONG
               : StopReason.DONT_STOP );
    }

    //LOG.debug( "checking item: {} ...runtime: {}m (max:{}m)", item, timed.toMinutes(), MAXDURR );
    if ( conv.isStatus( Status.RUNNING ) ) {
      if ( runTooLong ) {
        LOG.debug( "reaping work item {} (running too long)", conv.getItem() );
        return StopReason.TOO_LONG;
      }
      else {
        return StopReason.DONT_STOP;
      }
    }

    // if we exited, then we need to know why
    return ( conv.isStatus( Status.ERROR )
             ? StopReason.ERROR
             : StopReason.COMPLETED );
  }

  @Override
  public void shutdown() {
    shuttingDown = true;
  }

  public boolean isShuttingDown() {
    return shuttingDown;
  }

  public Reader getLog( Conversion conv, LogType type, boolean err ) throws IOException {
    File datadir = conv.getLogDir().toFile();
    if ( datadir.exists() && datadir.isDirectory() ) {
      throw new IOException( "Cannot locate log directory (or not a directory): " + datadir );
    }

    String gz = ( err
                  ? "stderr.gz"
                  : "stdout.gz" );

    return new InputStreamReader( new GZIPInputStream( new FileInputStream( new File( datadir, type + gz ) ) ) );
  }

  protected void saveLogs( Conversion conv, ProcessInfo proc, LogType type ) throws IOException {
    File datadir = conv.getLogDir().toFile();
    boolean ok = ( datadir.exists() && datadir.isDirectory()
                   ? true
                   : datadir.mkdirs() );
    if ( !ok ) {
      throw new RuntimeException( "unable to save logs: " + datadir );
    }

    // FIXME: check conv.compresslogs before compressing
    try ( Reader input = new FileReader( proc.stderrfile );
          OutputStream outstream = new GZIPOutputStream( new FileOutputStream( new File( datadir, type + "-stderr.gz" ) ) ) ) {
      IOUtils.copy( input, outstream, StandardCharsets.UTF_8.toString() );
    }

    try ( Reader input = new FileReader( proc.stdoutfile );
          OutputStream outstream = new GZIPOutputStream( new FileOutputStream( new File( datadir, type + "-stdout.gz" ) ) ) ) {
      IOUtils.copy( input, outstream, StandardCharsets.UTF_8.toString() );
    }
  }
}
