/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   H5Cat.cpp
 * Author: ryan
 * 
 * Created on February 28, 2018, 10:31 AM
 */

#include "H5Cat.h"
#include <H5Cpp.h>

H5Cat::H5Cat( const std::string& outfile ) : output( outfile ) {
}

H5Cat::H5Cat( const H5Cat& orig ) : output( orig.output ) {
}

H5Cat::~H5Cat( ) {
}

std::unique_ptr<H5::H5File> H5Cat::cat( std::vector<std::string>& filesToCat ) {
  std::unique_ptr<H5::H5File> h5( new H5::H5File( output, H5F_ACC_TRUNC ) );

  // we need to open all the files and see which datasets we have
  // since when we create the new datasets, we need to


  return std::move( h5 );
}

std::unique_ptr<H5::H5File> H5Cat::cat( const std::string& outfile,
    std::vector<std::string>& filesToCat ) {
  return H5Cat( outfile ).cat( filesToCat );
}
