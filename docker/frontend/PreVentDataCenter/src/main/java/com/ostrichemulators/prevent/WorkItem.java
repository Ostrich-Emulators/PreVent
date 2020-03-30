/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import java.nio.file.Path;
import java.time.LocalDateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public class WorkItem {

	private static final Logger LOG = LoggerFactory.getLogger( WorkItem.class );
	private Path dir;
	private String containerId;
	private LocalDateTime started;
	private LocalDateTime finished;

	public WorkItem() {
	}

	public WorkItem( Path dir ) {
		this.dir = dir;
	}

	public WorkItem( Path dir, String containerId, LocalDateTime started, LocalDateTime finished ) {
		this.dir = dir;
		this.containerId = containerId;
		this.started = started;
		this.finished = finished;
	}

	public Path getDir() {
		return dir;
	}

	public void setDir( Path dir ) {
		this.dir = dir;
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
