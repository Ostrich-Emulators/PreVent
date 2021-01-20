/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.Conversion;
import com.ostrichemulators.prevent.WorkItem.Status;
import com.ostrichemulators.prevent.conversion.Converter.StopReason;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public abstract class AbstractConverter implements Converter {

  private static final Logger LOG = LoggerFactory.getLogger( AbstractConverter.class );
  private boolean shuttingDown = false;

  protected StopReason conversionCanWaitLonger( Conversion conv ) {

    // if we're shutting down, we can't continue
    if ( shuttingDown ) {
      return StopReason.SHUTDOWN;
    }

    boolean runTooLong = conv.runOverlong();
    // if we're preprocessing just check running time
    if ( conv.isStatus( Status.PREPROCESSING ) && runTooLong ) {
      return StopReason.TOO_LONG;
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
}
