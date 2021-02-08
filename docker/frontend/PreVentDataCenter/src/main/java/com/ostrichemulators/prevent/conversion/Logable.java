/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent.conversion;

import java.nio.file.Path;

/**
 *
 * @author ryan
 */
public interface Logable {

  public Path getOut();

  public Path getErr();
}
