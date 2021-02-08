/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import java.io.File;
import java.nio.file.Path;

/**
 *
 * @author ryan
 */
public class ProcessInfo implements Logable {

  public final Process process;
  public final File dir;
  public final File stdoutfile;
  public final File stderrfile;

  public ProcessInfo( Process process, File dir, File stdoutfile, File stderrfile ) {
    this.process = process;
    this.dir = dir;
    this.stdoutfile = stdoutfile;
    this.stderrfile = stderrfile;
  }

  @Override
  public Path getOut() {
    return this.stdoutfile.toPath();

  }

  @Override
  public Path getErr() {
    return this.stderrfile.toPath();
  }
}
