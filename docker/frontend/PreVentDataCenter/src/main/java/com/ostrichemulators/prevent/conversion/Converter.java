/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.Conversion;
import com.ostrichemulators.prevent.WorkItem;
import java.util.concurrent.FutureTask;

/**
 *
 * @author ryan
 */
public interface Converter {

  public static enum LogType {
    STP, CONVERSION
  };

  static enum StopReason {
    TOO_LONG, ERROR, SHUTDOWN, COMPLETED, DONT_STOP
  };

  public void verifyAndPrepare();

  public Conversion reinitialize( Conversion item );

  public FutureTask<WorkItem> createTask( Conversion c, Object monitor );

  public void shutdown();
}
