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

Because input files can be very large, care has been taken to ensure that only as much data is read as is necessary. For example, the STP XML DOM is not read into memory at once; it is read piecemeal to populate internal data structures, and flushed as needed. If a large amount of data is needed before writing, extra data is cached to disk. This progressive reading and caching algorithm is part of all the readers, and keeps memory usage very low. The formatconverter API for creating readers and writers is documented in code only at this time.

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
--pattern | -p | format string | Set the output file naming pattern

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

The default output filename pattern is `%d%i.%t`, that is, the output filename is the input file name with a different extension. Note that some specifiers are similar to command-line options; these are separate concepts.

## HDF5 Native Format
The "native" output format for formatconverter is HDF5. We consider this "native" because it is the most feature-rich output format, and creates the smallest on-disk files. [HDF5](https://www.hdfgroup.org/solutions/hdf5/) is a flexible data format that is essentially a filesystem within a file. Data is organized in Groups and Datasets, an approach that is perfect for storing physiological data. 

The formatconverter's native format consists of three main Groups: _Events_, _VitalSigns_, and _Waveforms_, though _VitalSigns_ and _Waveforms_ use the exact same structure, and are separated merely for ease of use.

The file itself contains metadata useful for understanding/troubleshooting the data:
* **Build Number** The formatconverter that generated the file
* **Source Reader** The reader that generated the data
* **Filename** The original input file name
* **Layout Version** A number that denotes how the data is organized in the file. Should the layout change between versions, this number will orient the user/other tools to the actual format.
* **HDF5 Version** The HDF5 version for this file.

The file metadata, and all signal datasets contain timing information. At the file level, this metadata are the "global" values for all datasets (e.g., Start Time is the earliest start time of all the signals). Signals are not required to start and/or stop at the same time. 
* **Duration** The total duration for this dataset 
* **Start Time** The earliest time value contained in the dataset
* **Start Date/Time** an ISO8601 version of Start Time
* **End Time** The last time value contained in the dataset
* **End Date/Time** an ISO8601 version of End Time
* **Timezone** The timezone of Start Time and End Time

If the source input format supports metadata or attributes, these are duplicated in the file's metadata.

Lastly, every Dataset has a **Columns** attribute that describes the data each column of the dataset. An example of this attribute might be "timestamp (ms), segment offset", telling the user that the first column is a timestamp, and the second is something called "segment offset." All dataset columns have the same data type.

### Events
The Events Group contains one main dataset: _Global_Times_. This is a dataset containing a list of all times in the other datasets. This dataset is generally useful in conjunction with the `--time-step` option.
  
_Segment_Offsets_ may also exist in the _Events_ group. This is auxillary data provided by the STP XML reader.

### Signals
_VitalSigns_ and _Waveforms_ use the same structure, so they are described together here. A signal Group comprises two datasets: _data_ and _time_, plus metadata specific to the signal:
* **Data Label** The name of the signal. Signal names are cleaned to make the Dataset name easily useable by other tools. Things like spaces or punctuation are removed. This attribute provides the "raw" name of the signal.
* **Unit of Measure** The unit of the data points
* **Sample Period (ms)** What is the duration of a single unit of time?
* **Readings Per Sample** How many data points are present in a unit of time? In general, the difference between a waveform and a vital sign is this number: vital signs have 1 reading per sample, while waveforms have >1.

#### Data
The _data_ Dataset contains the data points for this signal. It is usually a single-column Dataset, but it can contain multiple columns if needed.  For example, some monitors provide "quality" metrics about each reading. _data_ Datasets always have an integer data type (short of regular). _data_-specific metadata include:
* **Missing Value Marker** A specific value that represents a missing value. This is useful primarily with waveform data.
* **Scale** A scaling factor used to convert floating-point numbers to integers. To calculate the "raw" value, divide the data point by 10<sup>scale</sup>
* **Min Value** The smallest raw data point value in the Dataset.
* **Max Value** The largest raw data point value in the Dataset.

As with the file metadata, if an input format supports per-signal metadata, it is duplicated in the _data_ Dataset.

#### Time
The _time_ Dataset contains the timing information the data points in _data_. It is always a single-column of long numbers. _time_ contains a single attribute to help users/tools interpret the data: **Time Source** is either _raw_ or _indexed_. If it is _raw_, the times in the column are actual times. If _indexed_, the times are index numbers to _Events/Global_Times_


