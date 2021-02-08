/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.ostrichemulators.prevent.WorkItem.Status;
import com.ostrichemulators.prevent.conversion.Converter.LogType;
import com.ostrichemulators.prevent.conversion.Logable;
import java.io.IOException;
import java.io.Reader;
import java.nio.file.Files;
import java.util.stream.Collectors;
import javafx.application.Platform;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.control.ToggleButton;
import javafx.scene.control.ToggleGroup;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * FXML Controller class
 *
 * @author ryan
 */
public class WorkItemEntryController {

  private static final Logger LOG = LoggerFactory.getLogger( WorkItemEntryController.class );
  private Conversion item;

  @FXML
  private Label txtlbl;

  @FXML
  private ToggleButton outbtn;

  @FXML
  private ToggleButton errbtn;

  @FXML
  private ToggleButton stpbtn;

  @FXML
  private ToggleGroup toggler;

  @FXML
  private ListView<String> listview;

  ChangeListener<Status> listener = ( ObservableValue<? extends Status> stat, Status older, Status newer ) -> {
    Platform.runLater( () -> {
      txtlbl.setText( item.getItem().getPath() + " - " + item.getItem().getStatus() );

      Logable l = item.getLogable();
      if ( null == l ) {
        stpbtn.setDisable( !Files.exists( item.getLog( LogType.STP, false ) ) );
        errbtn.setDisable( !Files.exists( item.getLog( LogType.CONVERSION, true ) ) );
        outbtn.setDisable( !Files.exists( item.getLog( LogType.CONVERSION, false ) ) );
      }
      else {
        if ( Status.PREPROCESSING == newer ) {
          stpbtn.setDisable( !Files.exists( l.getOut() ) );
          errbtn.setDisable( true );
          outbtn.setDisable( true );
        }
        else if ( Status.RUNNING == newer ) {
          stpbtn.setDisable( !Files.exists( item.getLog( LogType.STP, false ) ) );
          errbtn.setDisable( !Files.exists( l.getErr() ) );
          outbtn.setDisable( !Files.exists( l.getOut() ) );
        }
      }
    } );
  };

  public void setItem( Conversion conv ) {
    if ( null != this.item ) {
      this.item.getItem().statusProperty().removeListener( listener );
    }

    this.item = conv;
    listview.getItems().clear();
    toggler.selectToggle( null );

    item.getItem().statusProperty().addListener( listener );

    refreshBtns();
  }

  @FXML
  void refreshBtns() {
    if ( null != item ) {
      listener.changed( null, null, item.getItem().getStatus() );
    }
  }

  @FXML
  void showErrors( ActionEvent event ) {
    show( LogType.CONVERSION, true );
  }

  @FXML
  void showOutput( ActionEvent event ) {
    show( LogType.CONVERSION, false );
  }

  @FXML
  void showStp( ActionEvent event ) {
    show( LogType.STP, false );
  }

  private void show( LogType type, boolean err ) {

    // FIXME: if currently running, look at the logable
    try ( Reader input = item.getLogReader( type, err ) ) {
      listview.getItems().setAll( IOUtils.readLines( input ).stream().filter( s -> !s.isBlank() )
            .collect( Collectors.toList() ) );
      if ( listview.getItems().isEmpty() ) {
        listview.getItems().setAll( "No data to view" );
      }
    }
    catch ( IOException x ) {
      listview.getItems().setAll( "Error reading log file" );
    }
  }
}
