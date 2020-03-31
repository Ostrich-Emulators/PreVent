package com.ostrichemulators.prevent;

import java.io.IOException;
import java.net.URL;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Random;
import java.util.ResourceBundle;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;

public class PrimaryController implements Initializable {

	private static final int COLWIDTHS[] = { 15, 50, 15, 15 };
	@FXML
	private TableView<WorkItem> table;

	@FXML
	private TableColumn<WorkItem, ?> statuscol;

	@FXML
	private TableColumn<WorkItem, Path> filecol;

	@FXML
	private TableColumn<WorkItem, LocalDateTime> startedcol;

	@FXML
	private TableColumn<WorkItem, LocalDateTime> endedcol;

	private Worklist worklist;

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

		ObservableList<WorkItem> titems = table.getItems();

		worklist = Worklist.open( App.getConfigLocation() );
		titems.addAll( worklist.getItems() );

		filecol.setCellValueFactory( new PropertyValueFactory<>( "file" ) );

		startedcol.setCellValueFactory( new PropertyValueFactory<>( "started" ) );
		startedcol.setCellFactory( column -> new LocalDateTableCell() );

		endedcol.setCellValueFactory( new PropertyValueFactory<>( "finished" ) );
		endedcol.setCellFactory( column -> new LocalDateTableCell() );
	}

	@FXML
	private void switchToSecondary() throws IOException {
		App.setRoot( "secondary" );
	}

	@FXML
	private void addDir() throws IOException {
		Parent root1 = App.loadFXML( "workitementry" );
		Stage stage = new Stage();
		stage.initModality( Modality.APPLICATION_MODAL );
		stage.initStyle( StageStyle.DECORATED );
		stage.setTitle( "Create New Worklist Item" );
		stage.setScene( new Scene( root1 ) );
		stage.show();
	}

	private static class LocalDateTableCell extends TableCell<WorkItem, LocalDateTime> {

		@Override
		protected void updateItem( LocalDateTime item, boolean empty ) {
			super.updateItem( item, empty );
			if ( empty || null == item ) {
				setText( "empty" );
			}
			else {
				setText( item.format( DateTimeFormatter.ISO_DATE_TIME ) );
			}
		}
	}
}
