/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.io.IOException;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.stage.Stage;

/**
 * FXML Controller class
 *
 * @author ryan
 */
public class WorkitementryController {

	@FXML
	private Button cancelbtn;

	@FXML
	private Button savebtn;

	@FXML
	private void save() throws IOException {
		System.out.println( "save" );
		Stage stage = (Stage) savebtn.getScene().getWindow();
		stage.close();
	}

	@FXML
	private void cancel() throws IOException {
		System.out.println( "cancel" );
		Stage stage = (Stage) savebtn.getScene().getWindow();
		stage.close();
	}

}
