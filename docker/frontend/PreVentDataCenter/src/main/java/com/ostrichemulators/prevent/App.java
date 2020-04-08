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
import org.apache.commons.lang3.SystemUtils;

/**
 * JavaFX App
 */
public class App extends Application {

	private static Scene scene;
	static final DockerManager docker = DockerManager.connect();
	static final Preferences prefs = Preferences.userRoot().node( "com/ostrichemulators/prevent" );

	static final String NATIVESTP = "parser.stp.native";
	static final String STPISPHILIPS = "parser.stp.philips";
	static final String DOCKERCOUNT = "docker.containers.max";

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
