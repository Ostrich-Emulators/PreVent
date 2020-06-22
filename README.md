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



# Using/Building

The easiest way to run PreVent Tools is to use the docker images for [formatconverter](https://hub.docker.com/repository/docker/ry99/prevent) and [preventtools](https://hub.docker.com/repository/docker/ry99/prevent-tools). However, we have tried to make building the tools as easy as possible. If you are interested, we develop on Ubuntu, using NetBeans, though neither is required. 

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

# Formatconverter
Every file format organizes data differently. Sometimes data is stored sequentially; sometimes in parallel; sometimes in segments or chunks. With all the options available to designers, physiological data developers decided: "yes!" So, here we are.

At a high-level, formatconverter simply selects the "reader" to read the input, a "writer" to write the output, and then coordinates their operation. Each reader supports one or more input formats, and is responsible for organizing the data into standardized temporary data structures until enough data has been read to necessitate writing. The writer then flushes the data to one or more output files. Each input and output format may consist of one or more files. WFDB is an example of a format that spans multiple files.  

Because input files can be very large, care has been taken to ensure that only as much data is read as is necessary. For example, the STP XML DOM is not read into memory at once; it is read piecemeal to populate internal data structures, and flushed as needed. If a large amount of data is needed before writing, extra data is cached to disk. This progressive reading and caching algorithm is part of all the readers, and keeps memory usage very low. 

Also, the native output files are usually 15-60% smaller than the input file. 

## Features
 formatconverter has a number of features worth mentioning, though not all formats support all features:
* millisecond time resolution
* per-signal frequency
* per-signal auxillary data
* pre-signal arbitrary metadata
* arbitrary global metadata
* waveforms and vitals data are distinct
* "event" support
* cross-platform
* automatic input format resolution

Additionally, there are a number of optional features available:
* compression 
* local time/GMT time handling
* anonymization
* set arbitrary start date
* create SQLite database of metadata during conversion
* create new file per day/patient
* store timing information as offset from start date
* arbitrary output file naming conventions


## Command-Line options
formatconverter accepts a variety of command line options to enable features or change default behaviors. The table below describes these options

Long Option | Short Option | Valid Arguments {Default} | Description
---|---|---|---
--from | -f | wfdb, hdf5, stpxml, stpge, stpp, cpcxml, tdms, medi, dwc {auto} | Specify the input format
--to   | -t | wfdb, hdf5, mat4, mat7 | Specify the output format
--compression | -z | 0-9 {6} | Compression level
--sqlite| -s | db file | Create/Append SQLite metadata database
--quiet | -q | | Print less stuff to console
--stop-after-one | -1 | | Stop conversion after first file is generated. Useful for troubleshooting
--localtime | -l | | Convert times to local time
--offset | -Z | time string (MM/DD/YYYY) or seconds since 01/01/1970| Shift dates by the desired amount
--opening-date | -S | time string (MM/DD/YYYY) or seconds since 01/01/1970| Shift dates so that the first time in the output is the given date
--no-break or --one-file| -n | | Do not split output files by day
--no-cache| -C | | Do not cache anything to disk
--time-step| -T | | Store timing information as offset from start of file
--anonymize| -a  | | Attempt to anonymize the output files
--release| -R | | Show release information and exit
--pattern | -p | format string<sup>1</sup> | Set the output file naming pattern

### File Naming Format String
Because each input file can generate multiple output files, it is necessary to specify how those files should be named. This is accomplished using format specifiers within a string. The specifiers are:

**%p** - patient ordinal\
**%i** - input filename (without directory or extension)\
**%d** - input directory (with trailing separator)\
**%C** - current directory (with trailing separator)\
**%x** - input extension\
**%m** - modified date of input file\
**%c** - creation date of input file\
**%D** - date of conversion\
**%s** - date of first data point\
**%e** - date of last data point\
**%o** - output file ordinal\
**%t** - the --to option's extension (e.g., hdf5, wfdb)\
**%S** - same as `%d%i-p%p-%s.%t`

The default output filename pattern is `%d%i.%t`, that is, the output filename is the input file name with a different extension





## Native Format
formatconverter provides an API for generating files  






