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
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.commons.io.FilenameUtils;
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
				// check if this directory is WFDB, DWC, or ZL

				// DWC
				File[] inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "info" ) );
				if ( inners.length > 0 ) {
					return from( inners[0].toPath() );
				}

				// WFDB
				inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "hea" ) );
				if ( inners.length > 0 ) {
					return from( inners[0].toPath() );
				}

				// ZL
				inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "gzip" ) );
				if ( inners.length > 0 ) {
					return Optional.of( new WorkItem( p, DigestUtils.md5Hex( p.toAbsolutePath().toString() ), null, null, null ) );
				}
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

	public static List<WorkItem> recursively( Path p ) {
		List<WorkItem> items = new ArrayList<>();
		File f = p.toFile();
		if ( f.canRead() ) {
			if ( f.isDirectory() ) {
				// if we have a DWC, WFDB, or ZL directory, we can't recurse...
				// but if we have anything else, then recurse and add whatever we find.

				// DWC
				File[] inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "info" ) );
				if ( inners.length > 0 ) {
					from( inners[0].toPath() ).ifPresent( wi -> items.add( wi ) );
				}
				else {
					// WFDB
					inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "hea" ) );
					if ( inners.length > 0 ) {
						from( inners[0].toPath() ).ifPresent( wi -> items.add( wi ) );
					}
					else {
						// ZL
						inners = f.listFiles( fname -> FilenameUtils.isExtension( fname.getName(), "gzip" ) );
						if ( inners.length > 0 ) {
							from( p ).ifPresent( wi -> items.add( wi ) );
						}
						else {
							// f is a directory, so add all files we find there, and recurse
							// into all subdirectories
							for ( File sub : f.listFiles() ) {
								if ( sub.isDirectory() ) {
									items.addAll( recursively( sub.toPath() ) );
								}
								else {
									from( sub.toPath() ).ifPresent( wi -> items.add( wi ) );
								}
							}
						}
					}
				}
			}
			else {
				from( p ).ifPresent( wi -> items.add( wi ) );
			}
		}

		return items;
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
