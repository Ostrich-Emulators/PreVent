package com.ostrichemulators.prevent;

import java.io.IOException;
import java.net.URL;
import java.util.ResourceBundle;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;

public class PrimaryController implements Initializable {

	private static final int COLWIDTHS[] = { 15, 50, 15, 15 };
	@FXML
	private TableView<WorkItem> table;

	@FXML
	private TableColumn<?, ?> statuscol;

	@FXML
	private TableColumn<?, ?> filecol;

	@FXML
	private TableColumn<?, ?> startedcol;

	@FXML
	private TableColumn<?, ?> endedcol;

	@Override
	public void initialize( URL url, ResourceBundle rb ) {
		double sum = 0d;
		for ( int i : COLWIDTHS ) {
			sum += i;
		}

		TableColumn cols[] = { statuscol, filecol, startedcol, endedcol };
		for ( int i = 0; i < COLWIDTHS.length; i++ ) {
			double pct = COLWIDTHS[i] / sum;
			cols[i].prefWidthProperty().bind(
					table.widthProperty().multiply( pct ) );
		}
	}

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
		stage.initStyle( StageStyle.DECORATED );
		stage.setTitle( "Create New Worklist Item" );
		stage.setScene( new Scene( root1 ) );
		stage.show();
	}
}
