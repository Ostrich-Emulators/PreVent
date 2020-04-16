/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ostrichemulators.prevent;

/**
 *
 * @author ryan
 */
public interface Preference {

  static final String NATIVESTP = "parser.stp.native";
  static final String STPISPHILIPS = "parser.stp.philips";
  static final String DOCKERCOUNT = "docker.containers.max";
  static final String DOCKERREMOVE = "docker.containers.remove-on-success";
  static final String CONVERSIONLIMIT = "conversion.duration.maxminutes";
  static final String STPDIR = "tools.parser.stptoxml";
  static final String LASTDIR = "filedir";
}
