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

H5Cat::H5Cat( const std::string& outfile ) : output( outfile ) {
}

H5Cat::H5Cat( const H5Cat& orig ) : output( orig.output ){
}

H5Cat::~H5Cat( ) {
}

std::unique_ptr<H5::H5File> H5Cat::cat( const std::string& outfile,
        std::vector<std::string>& filesToCat ){
  
}
