/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * FXML Controller class
 *
 * @author ryan
 */
public class WorkItemEntryController {

  private static final Logger LOG = LoggerFactory.getLogger( WorkItemEntryController.class );
  private WorkItem item;

  @FXML
  private Label txtlbl;

  public void setItem( WorkItem wi ) {
    this.item = wi;
    txtlbl.setText( item.toString() );
  }

  @FXML
  void showErrors( ActionEvent event ) {
    LOG.debug( "show errors: {}", item );
  }

  @FXML
  void showOutput( ActionEvent event ) {
    LOG.debug( "show output: {}", item );

  }

  @FXML
  void showStp( ActionEvent event ) {
    LOG.debug( "show stp: {}", item );

  }

}
