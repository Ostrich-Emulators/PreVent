/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ZipReader.h
 * Author: ryan
 *
 * Created on February 14, 2018, 11:14 AM
 */

#ifndef ZIPREADER_H
#define ZIPREADER_H

#include "Reader.h"
#include <minizip/unzip.h>

class ZipReader : public Reader {
  public:

    ZipReader( );
	virtual ~ZipReader( );
};


#endif /* ZIPREADER_H */

