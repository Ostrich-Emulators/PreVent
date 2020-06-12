/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BSICache.h
 * Author: ryan
 *
 * Created on June 2, 2020, 8:54 AM
 */

#ifndef BSICache_H
#define BSICache_H

#include <string>
#include "FileCachingVector.h"

namespace FormatConverter {

  class BSICache : public FileCachingVector<double> {
  public:
    BSICache( const std::string& name = "" );
    virtual ~BSICache( );

    void name( const std::string& name );
    const std::string& name( ) const;
  private:
    std::string _name;
  };
}
#endif /* BSICache_H */

