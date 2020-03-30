package com.ostrichemulators.prevent;

import java.io.IOException;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;

public class PrimaryController {

	@FXML
	private void switchToSecondary() throws IOException {
		App.setRoot( "secondary" );
	}

	@FXML
	private void addDir() throws IOException {
		FXMLLoader fxmlLoader = new FXMLLoader( getClass().getResource( "workitementry.fxml" ) );
		Parent root1 = (Parent) fxmlLoader.load();
		Stage stage = new Stage();
		stage.initModality( Modality.APPLICATION_MODAL );
		stage.initStyle( StageStyle.DECORATED);
		stage.setTitle( "Create New Worklist Item");
		stage.setScene( new Scene( root1 ) );
		stage.show();
	}
}
