Dependencies to build these programs (ubuntu)

* g++
* pkg-config
* libhdf5-dev
* libmatio-dev
* libexpat-dev
* wfdb-10.6.0 (https://www.physionet.org/physiotools/wfdb-linux-quick-start.shtml)
* TDMSpp (https://github.com/Ostrich-Emulators/TDMSpp)
* googletest 1.8.0 (for unit testing [optional]) (https://github.com/google/googletest)

handy:
* hdf5-tools

This package uses CMake. To install the software:
# cmake CMakeLists.txt
# make
# sudo make install
