/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import javafx.fxml.FXML;
import javafx.scene.control.Label;

/**
 * FXML Controller class
 *
 * @author ryan
 */
public class WorkItemEntryController {

  private WorkItem item;

  @FXML
  private Label txtlbl;

  public void setItem( WorkItem wi ) {
    this.item = wi;
    txtlbl.setText( item.toString() );
  }
}
