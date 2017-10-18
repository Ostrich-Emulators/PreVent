/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Base64.h
 * Author: ryan
 *
 * Created on September 20, 2017, 10:12 AM
 * 
 * Largely copied from:
 * https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
 *
 */

#ifndef BASE64_H
#define BASE64_H

#include <vector>
#include <string>

typedef unsigned char BYTE;
std::string base64_encode(BYTE const* buf, unsigned int bufLen);
std::vector<BYTE> base64_decode(std::string const&);

#endif /* BASE64_H */

