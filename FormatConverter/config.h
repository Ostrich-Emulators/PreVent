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

#endif /* CONFIG_H */

