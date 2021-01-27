/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.ostrichemulators.prevent.WorkItem.Status;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Objects;

/**
 *
 * @author ryan
 */
public class Conversion {

  private Duration maxlifespan;
  private WorkItem item;
  private Path xmlpath;
  private Path logdir;
  private boolean compresslogs;
  private final List<WorkItemStateChangeListener> listeners = new ArrayList<>();

  private Conversion() {
  }

  public static ConversionBuilder builder( WorkItem item ) {
    return new Conversion().new ConversionBuilder().item( item );
  }

  public Duration getMaxLifespan() {
    return maxlifespan;
  }

  public WorkItem getItem() {
    return item;
  }

  public Path getXmlPath() {
    return xmlpath;
  }

  public boolean isStatus( Status s ) {
    return s.equals( item.getStatus() );
  }

  public boolean runOverlong() {
    return LocalDateTime.now().isAfter( item.getStarted().plus( maxlifespan ) );
  }

  public boolean needsStpToXml() {
    return ( Objects.isNull( xmlpath )
             ? false
             : !Files.exists( xmlpath ) );
  }

  public Path getLogDir() {
    return logdir;
  }

  public boolean isCompressLogs() {
    return compresslogs;
  }

  public void tellListeners() {
    listeners.stream().forEach( l -> l.itemChanged( item ) );
  }

  public class ConversionBuilder {

    private ConversionBuilder() {
    }

    public ConversionBuilder maxRuntime( Duration d ) {
      maxlifespan = d;
      return this;
    }

    public ConversionBuilder item( WorkItem i ) {
      item = i;
      return this;
    }

    public ConversionBuilder withXml( Path path ) {
      xmlpath = path;
      return this;
    }

    public ConversionBuilder withLogsIn( Path logs ) {
      logdir = logs;
      return this;
    }

    public ConversionBuilder compressLogs( boolean compress ) {
      compresslogs = compress;
      return this;
    }

    public ConversionBuilder listener( WorkItemStateChangeListener l ) {
      listeners.add( l );
      return this;
    }

    public ConversionBuilder setListeners( Collection<WorkItemStateChangeListener> ls ) {
      listeners.clear();
      listeners.addAll( ls );
      return this;
    }

    public Conversion build() {
      return Conversion.this;
    }
  }
}
