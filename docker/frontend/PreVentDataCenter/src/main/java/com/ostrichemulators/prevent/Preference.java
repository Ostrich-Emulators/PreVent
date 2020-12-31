/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.prefs.Preferences;

/**
 *
 * @author ryan
 */
public class Preference {

  private static final String NATIVESTP = "parser.stp.native";
  private static final String STPISPHILIPS = "parser.stp.philips";
  private static final String DOCKERCOUNT = "docker.containers.max";
  private static final String DOCKERREMOVE = "docker.containers.remove-on-success";
  private static final String CONVERSIONLIMIT = "conversion.duration.maxminutes";
  private static final String STPDIR = "tools.parser.stptoxml";
  private static final String LASTDIR = "directory.last";
  private static final String OUTPUTDIR = "directory.output";
  private static final String NATIVECONVERTER = "converter.native";

  private final Preferences prefs = Preferences.userRoot().node( "com/ostrichemulators/prevent" );

  private static Preference single;

  public static Preference open() {
    if ( null == single ) {
      single = new Preference();
    }
    return single;
  }

  public boolean useNativeStp() {
    return prefs.getBoolean( NATIVESTP, false );
  }

  public Preference setNativeStp( boolean yes ) {
    prefs.putBoolean( NATIVESTP, yes );
    return this;
  }

  public boolean useNativeConverter(){
    return prefs.getBoolean( NATIVECONVERTER, true );
  }

  public Preference setNativeConverter( boolean yes ){
    prefs.putBoolean( NATIVECONVERTER, yes);
    return this;
  }

  public boolean isStpPhilips() {
    return prefs.getBoolean( STPISPHILIPS, false );
  }

  public Preference setStpPhilips( boolean philips ) {
    prefs.putBoolean( STPISPHILIPS, philips );
    return this;
  }

  public int getMaxDockerCount() {
    return prefs.getInt( DOCKERCOUNT, DockerManager.DEFAULT_MAX_RUNNING_CONTAINERS );
  }

  public Preference setMaxDockerCount( int max ) {
    prefs.putInt( DOCKERCOUNT, max );
    return this;
  }

  public boolean removeDockerOnSuccess() {
    return prefs.getBoolean( DOCKERREMOVE, true );
  }

  public Preference setRemoveDockerOnSuccess( boolean remove ) {
    prefs.putBoolean( DOCKERREMOVE, remove );
    return this;
  }

  public int getMaxConversionMinutes() {
    return prefs.getInt( CONVERSIONLIMIT, Integer.MAX_VALUE );
  }

  public Preference setMaxConversionMinutes( int max ) {
    prefs.putInt( CONVERSIONLIMIT, max );
    return this;
  }

  public File getStpDir() {
    String stpdir = prefs.get( STPDIR, null );
    return ( null == stpdir
             ? null
             : new File( stpdir ) );
  }

  public Preference setStpDir( Path dir ) {
    prefs.put( STPDIR, dir.toString() );
    return this;
  }

  public File getLastOpenedDir() {
    return new File( prefs.get( LASTDIR, "." ) );
  }

  public Preference setLastOpenedDir( File f ) {
    prefs.put( LASTDIR, f.toString() );
    return this;
  }

  public Path getOutputPath() {
    String out = prefs.get( OUTPUTDIR, null );
    return ( null == out
             ? null
             : Paths.get( out ) );
  }

  public Preference setOutputPath( Path f ) {
    prefs.put( OUTPUTDIR, f.toString() );
    return this;
  }
}
