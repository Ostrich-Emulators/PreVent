/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeParser.h"

dr_time TimeParser::parse(const std::string& timestr){
  return std::stol( timestr );
}