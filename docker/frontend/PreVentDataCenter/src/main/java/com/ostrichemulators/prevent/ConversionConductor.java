/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.ostrichemulators.prevent.Conversion.ConversionBuilder;
import com.ostrichemulators.prevent.conversion.Converter;
import com.ostrichemulators.prevent.conversion.DockerConverter;
import com.ostrichemulators.prevent.conversion.NativeConverter;
import java.io.IOException;
import java.nio.file.Path;
import java.time.Duration;
import java.util.Collection;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import org.apache.commons.io.FilenameUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class ConversionConductor {

  public static final int DEFAULT_MAX_RUNNING_CONTAINERS = 3;
  private static final Logger LOG = LoggerFactory.getLogger( ConversionConductor.class );
  private static final int CHECK_INTERVAL_S = 5;

  private int maxcontainers = DEFAULT_MAX_RUNNING_CONTAINERS;
  private ExecutorService executor = Executors.newFixedThreadPool( maxcontainers );
  private ScheduledExecutorService threadreaper;
  private ScheduledFuture<?> reaperop;
  private final Object monitor = new Object();
  private boolean usenative;
  private Converter converter;

  public boolean useNativeConverters( boolean yes ) {
    usenative = yes;
    try {
      converter = ( usenative
                    ? new NativeConverter()
                    : new DockerConverter() );

      converter.verifyAndPrepare();
      return true;
    }
    catch ( Exception x ) {
      LOG.error( "Error preparing converter", x );
    }
    return false;
  }

  public void setMaxRunners( int maxx ) {
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

  public int getMaxRunners() {
    return maxcontainers;
  }

  public void shutdown() {
    LOG.info( "shutting down conversion manager" );
    converter.shutdown();

    if ( null != threadreaper ) {
      threadreaper.shutdownNow();

      LOG.debug( "cancelling reaperop thread" );
      reaperop.cancel( true );
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

    // tell all our running threads to shutdown
    synchronized ( monitor ) {
      monitor.notifyAll();
    }
    LOG.debug( "done shutting down" );
  }

  private void startReaping() {
    // Our threadreaper just refreshes the container states every X seconds
    // and notifies the worker threads to see what happened. The worker threads
    // need to figure out for themselves if/how they should exit.

    LOG.debug( "starting the conversion reaper" );
    threadreaper = Executors.newSingleThreadScheduledExecutor();
    // start a thread to wake up threads every few seconds
    reaperop = threadreaper.scheduleAtFixedRate( () -> {
      synchronized ( monitor ) {
        monitor.notifyAll();
      }
    }, 0, CHECK_INTERVAL_S, TimeUnit.SECONDS );
  }

  private static Path xmlPathForStp( WorkItem item, Path logdir ) {
    Path inputfile = item.getPath();
    String filename = inputfile.getFileName().toString();
    String basename = FilenameUtils.removeExtension( filename );

    Path xmlfile = logdir.resolve( filename ).resolveSibling( basename + ".xml" );
    return xmlfile;
  }

  public static boolean needsStpToXmlConversion( WorkItem item ) {
    return ( "stp".equalsIgnoreCase( FilenameUtils.getExtension( item.getPath().toString() ) )
          && "stpxml".equalsIgnoreCase( item.getType() ) );
  }

  public void convert( Collection<WorkItem> work, WorkItemStateChangeListener l ) throws IOException {
    if ( null == threadreaper ) {
      startReaping();
    }

    for ( WorkItem item : work ) {
      item.queued();
      l.itemChanged( item );
      executor.submit( converter.createTask( createConversion( item, l ), monitor ) );
    }
  }

  public void reinitializeItems( Collection<WorkItem> items, WorkItemStateChangeListener l ) {
    items.stream().map( wi -> createConversion( wi, l ) )
          .forEach( conv -> converter.reinitialize( conv ) );
  }

  private Conversion createConversion( WorkItem item, WorkItemStateChangeListener l ) {
    Path logdir = App.prefs.getLogPath().resolve( item.getId() );

    ConversionBuilder builder = Conversion.builder( item )
          .listener( l )
          .withLogsIn( logdir )
          .maxRuntime( Duration.ofMinutes( App.prefs.getMaxConversionMinutes() ) );
    if ( needsStpToXmlConversion( item ) ) {
      builder.withXml( xmlPathForStp( item, logdir ) );
    }

    return builder.build();
  }
}
