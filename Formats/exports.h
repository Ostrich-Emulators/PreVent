/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   exports.h
 * Author: ryan
 *
 * Created on August 1, 2020, 8:04 AM
 */

#ifndef EXPORTS_H
#define EXPORTS_H


#if defined(_WIN32) || defined(_Win64)
#define FMTCNV_EXPORT __declspec(dllexport)
#else
#define FMTCNV_EXPORT
#endif

#endif /* EXPORTS_H */

