/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BSIConverter.h
 * Author: ryan
 *
 * Created on June 11, 2020, 7:25 AM
 */

#ifndef BSICONVERTER_H
#define BSICONVERTER_H

#include <filesystem>
#include <fstream>

#include "BSICache.h"

namespace FormatConverter {

  class BSIConverter {
  public:
    std::filesystem::path convert( const std::filesystem::path& file ) const;
  private:
    size_t slowlyCountLines( std::fstream& stream ) const;
    void write( std::vector<BSICache *>& data, const std::filesystem::path& outfile) const;
  };
}



#endif /* BSICONVERTER_H */

