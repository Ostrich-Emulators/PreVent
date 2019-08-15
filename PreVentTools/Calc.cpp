/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:  Calc.cpp
 * Author: Matt
 *
 * Created on August 14, 2019, 3:10 PM
 */

//TODO: Determine if there is missing data in group and add warning
//TODO: Read in chunks to preserve memory
//TODO: Figure out how to handle #include statements

#include <cstdlib>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <getopt.h>
#include <sys/stat.h>
#include <fstream>

#include <H5Cpp.h>
#include <H5Opublic.h>
#include <vector>
#include <algorithm>
#include <cmath>

#include "H5Cat.h"
#include "dr_time.h"
#include "Calc.h"

void Calculate(std::string filename, std::string operation, int window, std::string path){
      
    H5::H5File file = H5::H5File( filename, H5F_ACC_RDONLY );
    H5::Group grp = file.openGroup( path );
    
    //READ DATA
    
    H5::DataSet dataset = grp.openDataSet( "data" );
    H5::DataSpace filespace = dataset.getSpace();
    hsize_t dims[2];
    filespace.getSimpleExtentDims(dims);
    int data_out[(int)dims[0]][(int)dims[1]] = {0}; //Makes buffer the size of the DataSet
    dataset.read(data_out, H5::PredType::NATIVE_INT);
    std::vector<int> output;
    for(auto &rows: data_out){ 
      for(auto &x: rows){
        if(x!=0){
          output.push_back(x);
        }
      }
    }
    dataset.close();

    if(window !=0){
      //READ TIMES
      H5::DataSet ds = grp.openDataSet( "time" );
      H5::DataSpace dataspace = ds.getSpace( );
      hsize_t DIMS[2] = { };
      dataspace.getSimpleExtentDims( DIMS );
      const hsize_t ROWS = DIMS[0];
      const hsize_t COLS = DIMS[1];
      const hsize_t sizer = ROWS * COLS;
      long read[sizer] = { };
      ds.read( read, ds.getDataType( ) );
      std::vector<dr_time> times;
      times.reserve( sizer );
      for ( hsize_t i = 0; i < sizer; i++ ) {
        long l = read[i];
        times.push_back( l );
      }
      ds.close( );

      std::vector<int>::reverse_iterator rit_data = output.rbegin();
      std::vector<dr_time>::reverse_iterator rit_times = times.rbegin();
      
      double end_time = *rit_times - (window * 1000);
      double begin_time = *times.begin();
      if(end_time < begin_time){
        std::cout<<"Window exceeds file start time"<<std::endl;
        return;
      }
      std::vector<int> buffer;
      while (*rit_times >= end_time) {

        buffer.push_back(*rit_data);
        rit_data++;
        rit_times++;

      }
      output=buffer;
    }
   
    double stat = 0;
    
    if (operation == "avg" || operation == "std" || operation == "var"){
      double total = 0;
      int count = 0;
      for(auto x: output){
        total+=x;
        count++;
      }
      double average = total/count;
      if(operation == "std" || operation == "var"){
        total = 0;
        for(auto y: output){
          total += ((y-average) * (y-average));
        }
        stat = total / (count-1);
        if(operation == "std"){
          stat = std::sqrt(stat);
        }
      } else {
        stat = average;
      }
    } else if (operation == "range" || operation == "med"){
      
      sort(output.begin(),output.end());
      size_t size = output.size();
      if (operation == "med"){
        if(size % 2 ==0){
          stat = (output[size/2 -1] + output[size/2])/2;
        } else {
          stat = output[size/2];
        }
      } else {
        stat = output.back()-output.front();
      }
    } else {

      std::cout<<"Operation not recognized"<<std::endl;

    }
     

    std::cout<<operation<<" from: "<<path<<" for the last "<<window<<" seconds is: "<<stat<<std::endl;    
    
}
