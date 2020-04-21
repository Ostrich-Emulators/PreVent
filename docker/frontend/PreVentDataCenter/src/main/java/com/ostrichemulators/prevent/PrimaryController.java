package com.ostrichemulators.prevent;

import static com.ostrichemulators.prevent.App.docker;
import com.ostrichemulators.prevent.WorkItem.Status;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashSet;
import java.util.List;
import java.util.ResourceBundle;
import java.util.Set;
import java.util.stream.Collectors;
import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.Alert;
import javafx.scene.control.ButtonBar;
import javafx.scene.control.ButtonType;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.OverrunStyle;
import javafx.scene.control.Spinner;
import javafx.scene.control.SpinnerValueFactory;
import javafx.scene.control.SpinnerValueFactory.ListSpinnerValueFactory;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableRow;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.stage.Window;
import javafx.util.StringConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PrimaryController implements Initializable, WorkItemStateChangeListener {

  private static final Logger LOG = LoggerFactory.getLogger( PrimaryController.class );
  private static final int COLWIDTHS[] = {15, 50, 10, 10, 15, 15, 10, 10};
  @FXML
  private TableView<WorkItem> table;

  @FXML
  private TableColumn<WorkItem, Status> statuscol;

  @FXML
  private TableColumn<WorkItem, Path> filecol;

  @FXML
  private TableColumn<WorkItem, Path> outputcol;

  @FXML
  private TableColumn<WorkItem, LocalDateTime> startedcol;

  @FXML
  private TableColumn<WorkItem, LocalDateTime> endedcol;

  @FXML
  private TableColumn<WorkItem, String> messagecol;

  @FXML
  private TableColumn<WorkItem, Long> sizecol;

  @FXML
  private TableColumn<WorkItem, String> typecol;

  @FXML
  private TableColumn<WorkItem, String> containercol;

  @FXML
  private CheckBox nativestp;

  @FXML
  private CheckBox usephilips;

  @FXML
  private CheckBox removecontainers;

  @FXML
  private Spinner<Integer> dockercnt;

  @FXML
  private Spinner<Integer> durationtimer;

  @FXML
  private Label outputlbl;

  private Path savelocation;

  @FXML
  void saveconfig() {
    App.docker.setMaxContainers( dockercnt.getValue() );
    dockercnt.setValueFactory( new SpinnerValueFactory.IntegerSpinnerValueFactory( 1,
          10, dockercnt.getValue() ) );

    App.prefs
          .setMaxConversionMinutes( durationtimer.getValue() )
          .setNativeStp( nativestp.isSelected() )
          .setMaxDockerCount( dockercnt.getValue() )
          .setStpPhilips( usephilips.isSelected() )
          .setRemoveDockerOnSuccess( removecontainers.isSelected() );
    if ( !"From Input".equals( outputlbl.getText() ) ) {
      App.prefs.setOutputPath( Paths.get( outputlbl.getText() ) );
    }
  }

  private void loadPrefs() {
    nativestp.setSelected( App.prefs.useNativeStp() );
    usephilips.setSelected( App.prefs.isStpPhilips() );
    removecontainers.setSelected( App.prefs.removeDockerOnSuccess() );
    App.docker.setMaxContainers( App.prefs.getMaxDockerCount() );
    dockercnt.setValueFactory( new SpinnerValueFactory.IntegerSpinnerValueFactory( 1,
          10, App.docker.getMaxContainers() ) );

    outputlbl.setTextOverrun( OverrunStyle.CENTER_ELLIPSIS );
    Path outpath = App.prefs.getOutputPath();
    outputlbl.setText( null == outpath ? "From Input" : outpath.toString() );

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

    if ( docker.verifyAndPrepare() ) {
      LOG.debug( "Docker is ready!" );
    }
    else {
      Alert alert = new Alert( Alert.AlertType.ERROR );
      alert.setTitle( "Docker Images Missing" );
      alert.setHeaderText( "Docker is not Initialized Properly" );
      ButtonType exitbtn = new ButtonType( "Exit Application", ButtonBar.ButtonData.CANCEL_CLOSE );
      alert.getButtonTypes().setAll( exitbtn );
      alert.setContentText( "Docker may not be started, or the ry99/prevent image could not be pulled.\nOn Windows, Docker must be listening to port 2375." );
      alert.showAndWait();
      Platform.exit();
    }

    try {
      List<WorkItem> allitems = Worklist.open( savelocation );

      table.setRowFactory( tv -> {
        TableRow<WorkItem> row = new TableRow();
        row.setOnMouseClicked( event -> {
          if ( event.getClickCount() > 1 && !row.isEmpty() ) {
            popDetails( row.getItem() );
          }
        } );
        return row;
      } );

      App.docker.reinitializeItems( allitems, this );

      table.getItems().addAll( allitems );
    }
    catch ( IOException x ) {
      LOG.error( "{}", x );
    }
  }

  private void popDetails( WorkItem item ) {
    try {
      Parent parent = App.loadFXML( "workitementry" );

      Stage newWindow = new Stage();
      newWindow.setTitle( "Item Details" );
      newWindow.initModality( Modality.WINDOW_MODAL );
      newWindow.initStyle( StageStyle.DECORATED );
      newWindow.setScene( new Scene( parent ) );
      newWindow.show();
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

    TableColumn cols[] = {statuscol, filecol, sizecol, typecol, startedcol, endedcol, messagecol, containercol};
    for ( int i = 0; i < COLWIDTHS.length; i++ ) {
      double pct = COLWIDTHS[i] / sum;
      cols[i].prefWidthProperty().bind(
            table.widthProperty().multiply( pct ) );
    }

    statuscol.setCellValueFactory( new PropertyValueFactory<>( "status" ) );
    messagecol.setCellValueFactory( new PropertyValueFactory<>( "message" ) );
    typecol.setCellValueFactory( new PropertyValueFactory<>( "type" ) );
    containercol.setCellValueFactory( new PropertyValueFactory<>( "containerId" ) );

    sizecol.setCellValueFactory( new PropertyValueFactory<>( "bytes" ) );
    sizecol.setCellFactory( column -> new KbTableCell() );

    filecol.setCellValueFactory( new PropertyValueFactory<>( "path" ) );
    filecol.setCellFactory( column -> new LeadingEllipsisTableCell() );

    outputcol.setCellValueFactory( new PropertyValueFactory<>( "outputPath" ) );
    outputcol.setCellFactory( column -> new LeadingEllipsisTableCell() );

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
  void convertAll() throws IOException {
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
  void convertSelected() throws IOException {
    try {
      // run whatever's selected (except already-queued or already running items)
      Set<Status> workable = new HashSet<>( List.of( Status.RUNNING, Status.PREPROCESSING, Status.QUEUED ) );
      List<WorkItem> todo = table.getSelectionModel().getSelectedItems().stream()
            .filter( wi -> !workable.contains( wi.getStatus() ) )
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
    chsr.setInitialDirectory( App.prefs.getLastOpenedDir() );
    Window window = table.getScene().getWindow();
    final boolean nativestpx = App.prefs.useNativeStp();

    List<WorkItem> newitems = chsr.showOpenMultipleDialog( window ).stream()
          .map( file -> Worklist.from( file.toPath(), nativestpx ) )
          .filter( wi -> wi.isPresent() )
          .map( wi -> wi.get() )
          .collect( Collectors.toList() );
    if ( !newitems.isEmpty() ) {
      table.getItems().addAll( newitems );
      Worklist.save( table.getItems(), savelocation );
      App.prefs.setLastOpenedDir( newitems.get( 0 ).getPath().getParent().toFile() );
    }
  }

  @FXML
  void addDir() throws IOException {
    DirectoryChooser chsr = new DirectoryChooser();
    chsr.setTitle( "Create New Worklist Items from Directory" );
    chsr.setInitialDirectory( App.prefs.getLastOpenedDir() );
    Window window = table.getScene().getWindow();
    final boolean nativestpx = App.prefs.useNativeStp();

    File dir = chsr.showDialog( window );
    Worklist.recursively( dir.toPath(), nativestpx ).forEach( wi -> table.getItems().add( wi ) );
    Worklist.save( table.getItems(), savelocation );
    App.prefs.setLastOpenedDir( dir.getParentFile() );
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

  @FXML
  void selectOutputDir() {
    DirectoryChooser chsr = new DirectoryChooser();
    chsr.setTitle( "Select Output Directory" );

    Path outdir = App.prefs.getOutputPath();
    if ( null != outdir ) {
      chsr.setInitialDirectory( outdir.toFile() );
    }
    Window window = table.getScene().getWindow();

    File dir = chsr.showDialog( window );
    outputlbl.setText( dir.getAbsolutePath() );
  }

  @FXML
  void setOutputFromInput() {
    outputlbl.setText( "From Input" );
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

  private static class KbTableCell extends TableCell<WorkItem, Long> {

    @Override
    protected void updateItem( Long bytes, boolean empty ) {
      super.updateItem( bytes, empty );

      setAlignment( Pos.CENTER_RIGHT );
      if ( empty ) {
        setText( null );
      }
      else {
        setText( String.valueOf( bytes / 1000 ) );
      }
    }
  }
}
