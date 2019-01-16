/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   config.h
 * Author: rpb6eg
 *
 * Created on August 11, 2016, 7:35 AM
 */

#ifndef CONFIG_H
#define CONFIG_H

const char dirsep =
#ifdef __linux__
      '/';
#else
      '\\';
#endif

const std::string osname =
#ifdef _WIN32
      "Windows 32-bit";
#elif _WIN64
      "Windows 64-bit";
#elif __unix || __unix__
      "Unix";
#elif __APPLE__ || __MACH__
      "Mac OSX";
#elif __linux__
      "Linux";
#elif __FreeBSD__
      "FreeBSD";
#elif __CYGWIN__
      "Cygwin";
#else
      "Other";
#endif


const int FC_VERS_MAJOR = 4;
const int FC_VERS_MINOR = 0;
const int FC_VERS_MICRO = 8;

#endif /* CONFIG_H */

