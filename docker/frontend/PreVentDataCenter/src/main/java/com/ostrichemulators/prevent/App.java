package com.ostrichemulators.prevent;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.apache.commons.lang3.SystemUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * JavaFX App
 */
public class App extends Application {

  private static final Logger LOG = LoggerFactory.getLogger( App.class );

  private static Scene scene;
  static final ConversionConductor converter = new ConversionConductor();
  public static final Preference prefs = Preference.open();

  @Override
  public void start( Stage stage ) throws IOException {
    scene = new Scene( loadFXML( "primary" ) );
    stage.setScene( scene );
    stage.setTitle( "PreVent Data Center" );
    stage.show();
  }

  static void setRoot( String fxml ) throws IOException {
    scene.setRoot( loadFXML( fxml ) );
  }

  public static Parent loadFXML( String fxml ) throws IOException {
    return new FXMLLoader( App.class.getResource( fxml + ".fxml" ) ).load();
  }

  public static Path getRootLocation() {
    String userhome = System.getProperty( "user.home" );

    return ( SystemUtils.IS_OS_WINDOWS
             ? Paths.get( userhome, "Application Data", "Prevent Data Center" )
             : Paths.get( userhome, ".prevent" ) );
  }

  public static Path getConfigLocation() {
    return getRootLocation().resolve( "worklist.pdc" );
  }

  public static Path getDataDirLocation() {
    return getRootLocation().resolve( "data" );
  }

  public static void main( String[] args ) {
    launch();
  }

  @Override
  public void stop() throws Exception {
    super.stop();
    converter.shutdown();
  }
}
