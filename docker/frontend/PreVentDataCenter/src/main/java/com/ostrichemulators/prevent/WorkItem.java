/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.nio.file.Path;
import java.time.LocalDateTime;
import java.util.Objects;

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
    KILLED
  }

  private Path file;
  private String containerId;
  private LocalDateTime started;
  private LocalDateTime finished;
  private String checksum; // this is really the ID of this item
  private String type;
  private Status status = Status.ADDED;

  private WorkItem() {
  }

  WorkItem( Path file, String checksum, String containerId, LocalDateTime started,
        LocalDateTime finished, String type ) {
    setPath( file );
    this.containerId = containerId;
    this.started = started;
    this.finished = finished;
    this.checksum = checksum;
    this.type = type;
  }

  public String getType() {
    return type;
  }

  public Path getPath() {
    return file;
  }

  public void setPath( Path file ) {
    this.file = file;
  }

  public String getContainerId() {
    return containerId;
  }

  public void setContainerId( String containerId ) {
    this.containerId = containerId;
  }

  public LocalDateTime getStarted() {
    return started;
  }

  public void queued() {
    this.started = null;
    this.finished = null;
    this.containerId = null;
    this.status = Status.QUEUED;
  }

  public void killed() {
    this.status = Status.KILLED;
  }

  public void started( String containerid ) {
    this.started = LocalDateTime.now();
    this.finished = null;
    this.containerId = containerid;
    this.status = Status.RUNNING;
  }

  public LocalDateTime getFinished() {
    return finished;
  }

  public void finished( LocalDateTime finished ) {
    this.finished = finished;
    this.status = Status.FINISHED;
  }

  public void error( String err ) {
    this.finished = LocalDateTime.now();
    this.status = Status.ERROR;
  }

  public void reinit() {
    this.started = null;
    this.finished = null;
    this.containerId = null;
    this.status = Status.ADDED;
  }

  public String getChecksum() {
    return checksum;
  }

  public Status getStatus() {
    return status;
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
    return String.format( "%s {%s}", file, ( null == containerId
                                             ? ""
                                             : containerId.substring( 0, 12 ) ) );
  }
}
