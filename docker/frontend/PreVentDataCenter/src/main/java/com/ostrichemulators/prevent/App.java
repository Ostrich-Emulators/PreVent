package com.ostrichemulators.prevent;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.prefs.Preferences;
import javafx.application.Platform;
import javafx.scene.control.Alert;
import javafx.scene.control.ButtonBar;
import javafx.scene.control.ButtonType;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * JavaFX App
 */
public class App extends Application {

  private static final Logger LOG = LoggerFactory.getLogger( App.class );

  private static Scene scene;
  static final DockerManager docker = DockerManager.connect();
  static final Preferences prefs = Preferences.userRoot().node( "com/ostrichemulators/prevent" );

  @Override
  public void start( Stage stage ) throws IOException {
    scene = new Scene( loadFXML( "primary" ) );
    stage.setScene( scene );
    stage.setTitle( "PreVent Data Center" );

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

    stage.show();
  }

  static void setRoot( String fxml ) throws IOException {
    scene.setRoot( loadFXML( fxml ) );
  }

  public static Parent loadFXML( String fxml ) throws IOException {
    return new FXMLLoader( App.class.getResource( fxml + ".fxml" ) ).load();
  }

  public static Path getConfigLocation() {
    String userhome = System.getProperty( "user.home" );

    return ( SystemUtils.IS_OS_WINDOWS
             ? Paths.get( userhome, "Application Data", "Prevent Data Center", "worklist.pdc" )
             : Paths.get( userhome, ".prevent", "worklist.pdc" ) );
  }

  public static void main( String[] args ) {
    launch();
  }

  @Override
  public void stop() throws Exception {
    super.stop();
    docker.shutdown();
  }
}
