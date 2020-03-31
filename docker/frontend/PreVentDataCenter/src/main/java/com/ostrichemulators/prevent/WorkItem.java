/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.util.Objects;
import java.util.Optional;
import org.apache.commons.codec.digest.DigestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author ryan
 */
public final class WorkItem {

	public enum Status {
		ADDED,
		STARTED,
		FINISHED,
		ERROR
	}

	private static final Logger LOG = LoggerFactory.getLogger( WorkItem.class );
	private Path file;
	private String containerId;
	@JsonFormat( shape = JsonFormat.Shape.STRING, pattern = "dd-MM-yyyy hh:mm:ss.SSS" )
	private LocalDateTime started;
	@JsonFormat( shape = JsonFormat.Shape.STRING, pattern = "dd-MM-yyyy hh:mm:ss.SSS" )
	private LocalDateTime finished;
	private String checksum; // this is really the ID of this item

	private WorkItem() {
	}

	private WorkItem( Path file, String checksum, String containerId, LocalDateTime started, LocalDateTime finished ) {
		setFile( file );
		this.containerId = containerId;
		this.started = started;
		this.finished = finished;
		this.checksum = checksum;
	}

	public static Optional<WorkItem> from( Path p ) {
		File f = p.toFile();

		if ( f.canRead() ) {
			if ( f.isDirectory() ) {
				return Optional.of( new WorkItem( p, DigestUtils.md5Hex( p.toAbsolutePath().toString() ), null, null, null ) );
			}
			else {
				try ( InputStream is = new BufferedInputStream( new FileInputStream( p.toFile() ) ) ) {
					return Optional.of( new WorkItem( p, DigestUtils.md5Hex( is ), null, null, null ) );
				}
				catch ( IOException x ) {
					LOG.error( "{}", x );
				}
			}
		}
		return Optional.empty();
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

	public void start( String containerid ) {
		this.started = LocalDateTime.now();
		this.containerId = containerid;
	}

	public LocalDateTime getFinished() {
		return finished;
	}

	public void finish( LocalDateTime finished ) {
		this.finished = finished;
	}

	public void error( String err ) {
		this.finished = LocalDateTime.now();
		this.started = null; // to force "error" status
	}

	public String getChecksum() {
		return checksum;
	}

	@JsonIgnore
	public Status getStatus() {
		if ( null == started && null == finished ) {
			return Status.ADDED;
		}
		if ( null == finished ) {
			return Status.STARTED;
		}
		if ( null == started ) {
			// ugly, but let's see how it goes...remember to null start time if error occurs
			return Status.ERROR;
		}

		return Status.FINISHED;
	}

	@Override
	public int hashCode() {
		int hash = 3;
		hash = 13 * hash + Objects.hashCode( this.checksum );
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
		return Objects.equals( this.checksum, other.checksum );
	}
}
