package com.ostrichemulators.prevent;

import com.ostrichemulators.prevent.WorkItem.Status;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashSet;
import java.util.List;
import java.util.ResourceBundle;
import java.util.Set;
import java.util.prefs.Preferences;
import java.util.stream.Collectors;
import javafx.collections.FXCollections;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.CheckBox;
import javafx.scene.control.OverrunStyle;
import javafx.scene.control.Spinner;
import javafx.scene.control.SpinnerValueFactory;
import javafx.scene.control.SpinnerValueFactory.ListSpinnerValueFactory;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Window;
import javafx.util.StringConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PrimaryController implements Initializable, WorkItemStateChangeListener {

	private static final Logger LOG = LoggerFactory.getLogger( PrimaryController.class );
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

	@FXML
	private CheckBox nativestp;

	@FXML
	private CheckBox usephilips;

	@FXML
	private Spinner<Integer> dockercnt;
	@FXML
	private Spinner<Integer> durationtimer;
	private final Preferences prefs = Preferences.userRoot().node( "com/ostrichemulators/prevent" );
	private Path savelocation;

	@FXML
	void saveconfig() {
		App.docker.setMaxContainers( dockercnt.getValue() );
		dockercnt.setValueFactory( new SpinnerValueFactory.IntegerSpinnerValueFactory( 1,
				10, dockercnt.getValue() ) );
		prefs.putBoolean( Preference.NATIVESTP, nativestp.isSelected() );
		prefs.putBoolean( Preference.STPISPHILIPS, usephilips.isSelected() );
		prefs.putInt( Preference.DOCKERCOUNT, dockercnt.getValue() );
		prefs.putInt( Preference.CONVERSIONLIMIT, durationtimer.getValue() );
	}

	private void loadPrefs() {
		nativestp.setSelected( prefs.getBoolean( Preference.NATIVESTP, false ) );
		usephilips.setSelected( prefs.getBoolean( Preference.STPISPHILIPS, false ) );
		App.docker.setMaxContainers( prefs.getInt( Preference.DOCKERCOUNT,
				DockerManager.DEFAULT_MAX_RUNNING_CONTAINERS ) );
		dockercnt.setValueFactory( new SpinnerValueFactory.IntegerSpinnerValueFactory( 1,
				10, App.docker.getMaxContainers() ) );

		ListSpinnerValueFactory<Integer> vfac = new SpinnerValueFactory.ListSpinnerValueFactory<>(
				FXCollections.observableArrayList( 10, 30, 60, 180, 480, Integer.MAX_VALUE ) );
		vfac.setConverter( new StringConverter<Integer>() {
			@Override
			public String toString( Integer t ) {
				if ( t <= 60 ) {
					return String.format( "%d minutes", t );
				}
				if ( t < Integer.MAX_VALUE ) {
					return String.format( "%d hours", t / 60 );
				}
				return "Unlimited";
			}

			@Override
			public Integer fromString( String string ) {
				for ( Integer i : vfac.getItems() ) {
					if ( toString( i ).equals( string ) ) {
						return i;
					}
				}
				return Integer.MAX_VALUE;
			}
		} );
                vfac.setValue( Integer.MAX_VALUE );
		durationtimer.setValueFactory( vfac );
	}

	@Override
	public void initialize( URL url, ResourceBundle rb ) {
		savelocation = App.getConfigLocation();
		fixTableLayout();

		loadPrefs();

		if ( App.docker.verifyAndPrepare() ) {
			LOG.debug( "Docker is ready!" );
		}
		else {
			LOG.error( "Docker does not have the required images pulled" );
		}

		try {
			List<WorkItem> allitems = Worklist.open( savelocation );
			App.docker.reinitializeItems( allitems, this );

			table.getItems().addAll( allitems );
		}
		catch ( IOException x ) {
			LOG.error( "{}", x );
		}
	}

	private void fixTableLayout() {
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

		statuscol.setCellValueFactory( new PropertyValueFactory<>( "status" ) );

		filecol.setCellValueFactory( new PropertyValueFactory<>( "path" ) );
		filecol.setCellFactory( column -> new LeadingEllipsisTableCell() );

		startedcol.setCellValueFactory( new PropertyValueFactory<>( "started" ) );
		startedcol.setCellFactory( column -> new LocalDateTableCell() );

		endedcol.setCellValueFactory( new PropertyValueFactory<>( "finished" ) );
		endedcol.setCellFactory( column -> new LocalDateTableCell() );
	}

	@FXML
	void switchToSecondary() throws IOException {
		App.setRoot( "secondary" );
	}

	@FXML
	void convert() throws IOException {
		try {
			// ignore items that are already running, already finished, or already queued
			Set<Status> workable = new HashSet<>( List.of( Status.ERROR, Status.ADDED, Status.KILLED ) );
			List<WorkItem> todo = table.getItems().stream()
					.filter( wi -> workable.contains( wi.getStatus() ) )
					.collect( Collectors.toList() );
			App.docker.convert( todo, this );
		}
		catch ( IOException x ) {
			LOG.error( "{}", x );
		}
	}

	@FXML
	void addFiles() throws IOException {
		FileChooser chsr = new FileChooser();
		chsr.setTitle( "Create New Worklist Items" );
		Window window = table.getScene().getWindow();
		final boolean nativestpx = App.prefs.getBoolean( Preference.NATIVESTP, false );

		chsr.showOpenMultipleDialog( window ).stream()
				.map( file -> Worklist.from( file.toPath(), nativestpx ) )
				.filter( wi -> wi.isPresent() )
				.map( wi -> wi.get() )
				.forEach( wi -> table.getItems().add( wi ) );
		Worklist.save( table.getItems(), savelocation );

//		Parent dialog = App.loadFXML( "workitementry" );
//		Stage stage = new Stage();
//		stage.initModality( Modality.APPLICATION_MODAL );
//		stage.initStyle( StageStyle.DECORATED );
//		stage.setTitle( "Create New Worklist Item" );
//		stage.setScene( new Scene( dialog ) );
//		stage.show();
	}

	@FXML
	void addDir() throws IOException {
		DirectoryChooser chsr = new DirectoryChooser();
		chsr.setTitle( "Create New Worklist Items from Directory" );
		Window window = table.getScene().getWindow();
		final boolean nativestpx = App.prefs.getBoolean( Preference.NATIVESTP, false );

		File dir = chsr.showDialog( window );
		Worklist.recursively( dir.toPath(), nativestpx ).forEach( wi -> table.getItems().add( wi ) );
		Worklist.save( table.getItems(), savelocation );
	}

	@Override
	public void itemChanged( WorkItem item ) {
		table.refresh();
		try {
			Worklist.save( table.getItems(), savelocation );
		}
		catch ( IOException xx ) {
			LOG.warn( "Could not save WorkItem update", xx );
		}
	}

	private static class LocalDateTableCell extends TableCell<WorkItem, LocalDateTime> {

		@Override
		protected void updateItem( LocalDateTime item, boolean empty ) {
			super.updateItem( item, empty );
			if ( empty || null == item ) {
				setText( null );
			}
			else {
				setText( item.format( DateTimeFormatter.ofPattern( "MM/dd/yyyy hh:mm:ss.SSS" ) ) );
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
