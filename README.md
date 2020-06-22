# PreVent Tools
PreVent Tools is a set of tools designed to facilitate conversion of physiological data monitoring data to differnet formats. Physiological data monitors and their constellation of tools generally create data in specific formats, and interoperability is a problem. PreVent Tools seeks to provide conversion capabilities to/from a variety of formats, and also provide "native" format for archiving and sharing. 

Monitor/Format          | Input            | Output
------------------------|------------------|-------
HDF5 (PreVent Native)   | X                | X
WFDB                    | X                | X
STP XML (versions 6-8)  | X                |
STP (GE)                | X<sup>1</sup>    |
STP (Philips MX800)     | X<sup>2<sup>     |
Matlab 7.X              |                  | X
CPC                     | X                | 
Data Warehouse Connect  | X                |
TDMS                    | X                |
MEDI                    | X                | 

1 Experimental support\
2 Experimental support; waveforms not implemented

PreVent Tools comprises two tools at this time: `formatconverter` is a command-line tool for converting between formats, while `preventtools` provides several useful tools for working with the data. `preventtools` primarily works with the native HDF5 format.



# Features formatconverter
Every file format organizes data differently. Sometimes data is stored sequentially; sometimes in parallel; sometimes in segments or chunks. With all the options available to designers, physiological data developers decided: "yes!" So, here we are. formatconverter has a number of features worth mentioning, though not all formats support all features:
* millisecond time resolution
* per-signal frequency
* per-signal auxillary data
* pre-signal arbitrary metadata
* arbitrary global metadata
* waveforms and vitals data are distinct
* "event" support
* cross-platform

Additionally, there are a number of optional features available:
* compression 
* local time/GMT time handling
* anonymization
* set arbitrary start date
* create SQLite database of metadata during conversion
* create new file per day/patient
* store timing information as offset from start date
* arbitrary output file naming conventions

Finally, formatconverter uses very little memory, regardless of input file size or format, and output files are usually significantly smaller than the input file. 




# Building

The easiest way to run PreVent Tools is to use the docker images for [formatconverter](https://hub.docker.com/repository/docker/ry99/prevent) and [preventtools](https://hub.docker.com/repository/docker/ry99/prevent-tools). However, we have tried to make building the tools as easy as possible. If you are interested, we develop on ubuntu, using NetBeans, though neither is required. 

Dependencies to build these programs (ubuntu)

* g++
* pkg-config
* libhdf5-dev
* libmatio-dev
* libexpat-dev
* wfdb-10.6.0 (https://archive.physionet.org/physiotools/wfdb-linux-quick-start.shtml)
* TDMSpp (https://github.com/Ostrich-Emulators/TDMSpp)
* googletest 1.8.0 (for unit testing [optional]) (https://github.com/google/googletest)

handy:
* hdf5-tools

This package uses CMake. To install the software:
```
> cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
> make
> sudo make install
```







