/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDateTime;
import java.util.Objects;
import java.util.UUID;
import javafx.beans.property.ReadOnlyLongProperty;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.beans.property.ReadOnlyStringProperty;
import javafx.beans.property.SimpleLongProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.beans.property.SimpleStringProperty;

/**
 *
 * @author ryan
 */
public final class WorkItem {

  public enum Status {
    ADDED,
    QUEUED,
    RUNNING,
    FINISHED,
    ERROR,
    KILLED,
    PREPROCESSING
  }

  private Path file;
  private String id;

  private final SimpleLongProperty bytes = new SimpleLongProperty();
  private final SimpleStringProperty containerId = new SimpleStringProperty();
  private final SimpleObjectProperty<LocalDateTime> started = new SimpleObjectProperty<>();
  private final SimpleObjectProperty<LocalDateTime> finished = new SimpleObjectProperty<>();
  private final SimpleStringProperty type = new SimpleStringProperty();
  private final SimpleObjectProperty<Status> status = new SimpleObjectProperty<>( Status.ADDED );
  private final SimpleStringProperty message = new SimpleStringProperty();
  private final SimpleObjectProperty<Path> outputdir = new SimpleObjectProperty<>();

  // for jackson
  WorkItem() {
  }

  public static WorkItemBuilder builder( Path p ) {
    return new WorkItemBuilder( p );
  }

  private WorkItem( Path file, String id, String containerId, LocalDateTime started,
        LocalDateTime finished, String type, long size, Path outputdir ) {
    setPath( file );
    this.containerId.set( containerId );
    this.started.set( started );
    this.finished.set( finished );
    this.id = ( null == id
                ? UUID.randomUUID().toString()
                : id );
    this.type.set( type );
    this.bytes.set( size );
    this.outputdir.set( outputdir );
  }

  public ReadOnlyStringProperty typeProperty() {
    return type;
  }

  public String getType() {
    return type.get();
  }

  public void setType( String t ) {
    this.type.set( t );
  }

  public Path getPath() {
    return file;
  }

  public void setPath( Path file ) {
    this.file = file;
  }

  public Path getOutputPath() {
    return outputdir.get();
  }

  public void setOutputPath( Path file ) {
    this.outputdir.set( file );
  }

  public ReadOnlyObjectProperty<Path> outputPathProperty() {
    return outputdir;
  }

  public ReadOnlyStringProperty containerIdProperty() {
    return containerId;
  }

  public String getContainerId() {
    return containerId.get();
  }

  public void setContainerId( String containerId ) {
    this.containerId.set( containerId );
  }

  public LocalDateTime getStarted() {
    return started.get();
  }

  public long getBytes() {
    return bytes.get();
  }

  public void setBytes( long bytes ) {
    this.bytes.set( bytes );
  }

  public ReadOnlyLongProperty bytesProperty() {
    return bytes;
  }

  public void queued() {
    this.started.set( null );
    this.finished.set( null );
    this.containerId.set( null );
    this.message.set( null );
    this.status.set( Status.QUEUED );
  }

  public void killed() {
    this.status.set( Status.KILLED );
  }

  public void preprocess() {
    this.started.set( LocalDateTime.now() );
    this.finished.set( null );
    this.containerId.set( null );
    this.status.set( Status.PREPROCESSING );
  }

  public void started( String containerid ) {
    if ( Status.PREPROCESSING != this.status.get() ) {
      this.started.set( LocalDateTime.now() );
    }
    this.finished.set( null );
    this.containerId.set( containerid );
    this.status.set( Status.RUNNING );
  }

  void setStarted( LocalDateTime f ) {
    this.started.set( f );
  }

  public LocalDateTime getFinished() {
    return finished.get();
  }

  public void finished( LocalDateTime finished ) {
    this.finished.set( finished );
    this.status.set( Status.FINISHED );
  }

  void setFinished( LocalDateTime f ) {
    this.finished.set( f );
  }

  public ReadOnlyObjectProperty<LocalDateTime> finishedProperty() {
    return finished;
  }

  public ReadOnlyObjectProperty<LocalDateTime> startedProperty() {
    return started;
  }

  public void error( String err ) {
    this.finished.set( LocalDateTime.now() );
    this.status.set( Status.ERROR );
    this.message.set( err );
  }

  public void reinit() {
    this.started.set( null );
    this.finished.set( null );
    this.containerId.set( null );
    this.message.set( null );
    this.status.set( Status.ADDED );
  }

  public String getId() {
    return id;
  }

  public ReadOnlyObjectProperty<Status> statusProperty() {
    return status;
  }

  public Status getStatus() {
    return status.get();
  }

  void setStatus( Status s ) {
    status.set( s );
  }

  public String getMessage() {
    return message.get();
  }

  void setMessage( String s ) {
    message.set( s );
  }

  public ReadOnlyStringProperty messageProperty() {
    return message;
  }

  @Override
  public int hashCode() {
    int hash = 5;
    hash = 23 * hash + Objects.hashCode( this.file );
    hash = 23 * hash + Objects.hashCode( this.containerId );
    hash = 23 * hash + Objects.hashCode( this.status );
    return hash;
  }

  @Override
  public boolean equals( Object obj ) {
    if ( this == obj ) {
      return true;
    }
    if ( obj == null ) {
      return false;
    }
    if ( getClass() != obj.getClass() ) {
      return false;
    }
    final WorkItem other = (WorkItem) obj;
    if ( !Objects.equals( this.containerId, other.containerId ) ) {
      return false;
    }
    if ( !Objects.equals( this.file, other.file ) ) {
      return false;
    }
    if ( this.status != other.status ) {
      return false;
    }
    return true;
  }

  @Override
  public String toString() {
    String cid = containerId.getValueSafe();
    String c = ( cid.isBlank()
                 ? "none"
                 : cid.substring( 0, Math.min( cid.length(), 12 ) ) );
    return String.format( "%s {%s}", file, c );
  }

  public static class WorkItemBuilder {

    private Path file;
    private String type;
    private Path outputdir;
    private long size;

    private WorkItemBuilder( Path p ) {
      file = p;
    }

    /**
     * Sets the path of this builder. Note that this function could reset the
     * output directory if the output directory is derived from the input file
     *
     * @param f
     * @return
     */
    public WorkItemBuilder path( Path f ) {
      file = f;
      return this;
    }

    public WorkItemBuilder type( String t ) {
      type = t;
      return this;
    }

    /**
     * Set the output location.
     *
     * @param p The new location. If null, the output location is derived from
     * the input path.
     * @return
     */
    public WorkItemBuilder outdir( Path p ) {
      outputdir = p;
      return this;
    }

    /**
     * Determines the appropriate output location based on the string path.
     *
     * @param s the output location. If null, the outdir will be derived from
     * the input path.
     * @return
     */
    public WorkItemBuilder calculateOutput( String s ) {
      return ( null == s
               ? outdir( null )
               : outdir( Paths.get( s ) ) );
    }

    public WorkItemBuilder bytes( long l ) {
      size = l;
      return this;
    }

    /**
     * The current output directory. For information purposes only :)
     *
     * @return
     */
    public Path currentOutputDir() {
      return ( null == outputdir
               ? file.getParent()
               : outputdir );
    }

    public WorkItem build() {
      return new WorkItem( file, null, null, null, null, type, size, currentOutputDir() );
    }
  }
}
