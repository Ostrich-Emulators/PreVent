/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import com.ostrichemulators.prevent.Conversion;
import com.ostrichemulators.prevent.WorkItem;
import com.ostrichemulators.prevent.WorkItem.Status;
import java.io.File;
import java.io.IOException;
import java.time.LocalDateTime;
import java.util.Set;
import java.util.concurrent.FutureTask;
import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class NativeConverter extends AbstractConverter {

  private static final Logger LOG = LoggerFactory.getLogger( NativeConverter.class );
  private static Set<Status> REINIT_STATUSES = Set.of( Status.QUEUED, Status.PREPROCESSING, Status.RUNNING );

  @Override
  public void verifyAndPrepare() {
    // extract the right native converter for the current OS
  }

  @Override
  public void reinitialize( Conversion item ) {
    // native processor only works while this app is running, so
    // any item not completed or errored must be reinitialized
    if ( REINIT_STATUSES.contains( item.getItem().getStatus() ) ) {
      item.getItem().reinit();
    }
  }

  @Override
  public FutureTask<WorkItem> createTask( Conversion item, Object monitor ) {
    return new FutureTask<>( () -> {
      ProcessInfo cnv = null;
      cnv = null;
      boolean doxml = item.needsStpToXml();
      try {
        File f = item.getLogDir().toFile();
        if ( !f.exists() ) {
          f.mkdirs();
        }

        if ( doxml ) {
          item.getItem().preprocess();
          item.tellListeners();

          LOG.debug( "Calling StpToolkit on {} (xml:{})", item.getItem().getPath(), item.getXmlPath() );
          cnv = StpToXml.convert( item.getItem().getPath(), item.getXmlPath() );
          synchronized ( monitor ) {
            while ( cnv.process.isAlive() && StopReason.DONT_STOP == conversionCanWaitLonger( item ) ) {
              try {
                monitor.wait();
              }
              catch ( InterruptedException ie ) {
                if ( isShuttingDown() ) {
                  cnv.process.destroyForcibly();
                }
                else {
                  LOG.warn( "ignoring this interruption (preprocessing): {}", ie.getLocalizedMessage() );
                }
              }
            }
          }

          int ret = cnv.process.exitValue();
          this.saveLogs( item, cnv, LogType.STP );

          if ( 0 != ret ) {
            // FIXME: save the output log somewhere
            item.getItem().error( "stp conversion failed" );
            item.tellListeners();
            return;
          }

          FileUtils.deleteDirectory( cnv.dir );
        }

        // done with preprocessing, so start the actual conversion
        ProcessInfo fmtcnv = FormatConverter.convert( item, doxml
                                                            ? item.getXmlPath()
                                                            : null );
        item.getItem().started( "" ); // FIXME: need some sort of container id here
        item.tellListeners();

        synchronized ( monitor ) {
          StopReason reason = StopReason.DONT_STOP;
          while ( StopReason.DONT_STOP == reason ) {
            try {
              monitor.wait();
              reason = this.conversionCanWaitLonger( item, fmtcnv );
            }
            catch ( InterruptedException ie ) {
              if ( isShuttingDown() ) {
                fmtcnv.process.destroyForcibly();
              }
              else {
                LOG.warn( "ignoring this interruption: {}", ie.getLocalizedMessage() );
              }
            }
          }

          // okay, we can't wait any longer.
          // this means one of 4 things:
          // 1) the conversion has completed normally
          // 2) the conversion ended in error
          // 3) the conversion was killed (timed out)
          // 4) the system is shutting down
          // FIXME: no matter how we got to this point, save the container logs somewhere
          switch ( reason ) {
            case TOO_LONG:
              item.getItem().killed();
              break;
            case ERROR:
              item.getItem().error( "process died in error" );
              break;
            case COMPLETED:
              item.getItem().finished( LocalDateTime.now() );
              break;
            case SHUTDOWN:
              // don't do anything here (the container is still running)
              break;
            default:
              // we should never get here!
              throw new IllegalStateException( "BUG! Unhandled stop reason: " + reason );
          }
        }

        this.saveLogs( item, fmtcnv, LogType.CONVERSION );
        FileUtils.deleteDirectory( fmtcnv.dir );
        FileUtils.deleteQuietly( fmtcnv.stderrfile );
        FileUtils.deleteQuietly( fmtcnv.stdoutfile );
        LOG.debug( "done waiting! " );
      }
      catch ( IOException x ) {
        LOG.error( "{}", x );
        item.getItem().error( x.getMessage() );
      }

      finally {
        item.tellListeners();

        if ( doxml ) {
          LOG.debug( "removing XML from STPtoXML conversion: {}", item.getXmlPath() );
          item.getXmlPath().toFile().delete();
        }
      }
    }, item.getItem() );
  }

  private StopReason conversionCanWaitLonger( Conversion conv, ProcessInfo process ) {
    if ( !process.process.isAlive() ) {
      int ret = process.process.exitValue();
      if ( 0 == ret ) {
        conv.getItem().finished( LocalDateTime.now() );
      }
      else {
        conv.getItem().error( "processed died in error" );
      }
    }

    return super.conversionCanWaitLonger( conv );
  }
}
