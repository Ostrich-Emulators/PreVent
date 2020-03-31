package com.ostrichemulators.prevent;

import com.ostrichemulators.prevent.WorkItem.Status;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ResourceBundle;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.OverrunStyle;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Window;

public class PrimaryController implements Initializable {

	private static final int COLWIDTHS[] = { 15, 50, 15, 15 };
	@FXML
	private TableView<WorkItem> table;

	@FXML
	private TableColumn<WorkItem, Status> statuscol;

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

		worklist = Worklist.open( App.getConfigLocation() );
		table.getItems().addAll( worklist.getItems() );

		statuscol.setCellValueFactory( new PropertyValueFactory<>( "status" ) );

		filecol.setCellValueFactory( new PropertyValueFactory<>( "file" ) );
		filecol.setCellFactory( column -> new LeadingEllipsisTableCell() );

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
	private void addFiles() throws IOException {
		FileChooser chsr = new FileChooser();
		chsr.setTitle( "Create New Worklist Items" );
		Window window = table.getScene().getWindow();

		chsr.showOpenMultipleDialog( window ).stream()
				.map( file -> WorkItem.from( file.toPath() ) )
				.filter( wi -> wi.isPresent() )
				.map( wi -> wi.get() )
				.forEach( wi -> {
					if ( worklist.add( wi ) ) {
						table.getItems().add( wi );
					}
				} );

//		Parent dialog = App.loadFXML( "workitementry" );
//		Stage stage = new Stage();
//		stage.initModality( Modality.APPLICATION_MODAL );
//		stage.initStyle( StageStyle.DECORATED );
//		stage.setTitle( "Create New Worklist Item" );
//		stage.setScene( new Scene( dialog ) );
//		stage.show();
	}

	@FXML
	private void addDir() throws IOException {
		DirectoryChooser chsr = new DirectoryChooser();
		chsr.setTitle( "Create New Worklist Items from Directory" );
		Window window = table.getScene().getWindow();

		File dir = chsr.showDialog( window );
		WorkItem.from( dir.toPath() ).ifPresent( wi -> {
			if ( worklist.add( wi ) ) {
				table.getItems().add( wi );
			}
		} );

//		Parent dialog = App.loadFXML( "workitementry" );
//		Stage stage = new Stage();
//		stage.initModality( Modality.APPLICATION_MODAL );
//		stage.initStyle( StageStyle.DECORATED );
//		stage.setTitle( "Create New Worklist Item" );
//		stage.setScene( new Scene( dialog ) );
//		stage.show();
	}

	private static class LocalDateTableCell extends TableCell<WorkItem, LocalDateTime> {

		@Override
		protected void updateItem( LocalDateTime item, boolean empty ) {
			super.updateItem( item, empty );
			if ( empty || null == item ) {
				setText( null );
			}
			else {
				setText( item.format( DateTimeFormatter.ofPattern( "dd-MM-yyyy hh:mm:ss.SSS" ) ) );
			}
		}
	}

	private static class LeadingEllipsisTableCell extends TableCell<WorkItem, Path> {

		@Override
		protected void updateItem( Path item, boolean empty ) {
			super.updateItem( item, empty );

			setTextOverrun( OverrunStyle.LEADING_ELLIPSIS );
			if ( empty || null == item ) {
				setText( null );
			}
			else {
				setText( item.toString() );
			}
		}
	}

}
