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
}
