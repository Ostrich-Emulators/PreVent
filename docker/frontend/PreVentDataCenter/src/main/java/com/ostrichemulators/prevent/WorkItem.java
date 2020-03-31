/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.nio.file.Path;
import java.time.LocalDateTime;
import java.util.UUID;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class WorkItem {

	private static final Logger LOG = LoggerFactory.getLogger( WorkItem.class );
	private Path file;
	private String containerId;
	private LocalDateTime started;
	private LocalDateTime finished;
	private String checksum; // this is really the ID of this item

	public WorkItem() {
	}

	public WorkItem( Path file ) {
		this.file = file;
	}

	public WorkItem( Path file, String containerId, LocalDateTime started, LocalDateTime finished ) {
		this.file = file;
		this.containerId = containerId;
		this.started = started;
		this.finished = finished;
	}

	public Path getFile() {
		return file;
	}

	public void setFile( Path file ) {
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

	public void setStarted( LocalDateTime started ) {
		this.started = started;
	}

	public LocalDateTime getFinished() {
		return finished;
	}

	public void setFinished( LocalDateTime finished ) {
		this.finished = finished;
	}

}
