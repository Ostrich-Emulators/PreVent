/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   H5Cat.h
 * Author: ryan
 *
 * Created on February 28, 2018, 10:31 AM
 */

#ifndef H5CAT_H
#define H5CAT_H

#include <memory>
#include <vector>
#include <string>
#include "dr_time.h"

namespace H5 {
  class H5File;
}

class H5Cat {
public:
  H5Cat( const std::string& output );
  H5Cat( const H5Cat& orig );
  virtual ~H5Cat( );

  void setDuration( const dr_time& duration_ms, dr_time * start = nullptr );
  void setClipping( const dr_time& start, const dr_time& end );

  void cat( std::vector<std::string>& filesToCat );
  static void cat( const std::string& outfile, std::vector<std::string>& filesToCat );
private:
  static bool filesorter( const std::string& a, const std::string& b );

  const std::string output;

  bool doduration;
  bool havestart;
  bool doclip;
  dr_time duration_ms;
  dr_time start;
  dr_time end;
};

#endif /* H5CAT_H */

