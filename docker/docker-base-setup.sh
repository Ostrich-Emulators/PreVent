#!/bin/bash
	
# Matio has an ubuntu package, but we build it ourselves because of 
# inconsistencies in the packaging and CMAKE, and to avoid the dependency on HDF5 1.8 (we use 1.10)
git clone git://git.code.sf.net/p/matio/matio /tmp/matio
pushd /tmp/matio
git submodule update --init
./autogen.sh
./configure --prefix=/usr --with-hdf5=/usr/lib/x86_64-linux-gnu/hdf5/serial
make
make install
popd

# WFDB doesn't have an ubuntu package
curl --silent --create-dirs --output /tmp/wfdb.tgz https://archive.physionet.org/physiotools/wfdb.tar.gz
cd /tmp
tar xfz wfdb.tgz
pushd wfdb-10.6.2
./configure --prefix=/usr
make
make install
popd

# neither does the TDMS library
git clone https://github.com/Ostrich-Emulators/TDMSpp.git
pushd TDMSpp
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr CMakeLists.txt
make
make install
popd

# and finally, we can build the format converter
git clone https://github.com/Ostrich-Emulators/PreVent.git
pushd PreVent
git checkout XXVERSIONXX
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr CMakeLists.txt
make
make install
popd

ln -s /usr/lib/formatconverter-XXVERSIONXX/libformats.so /usr/lib

cd /tmp
rm -rf TDMSpp* PreVent* matio* wfdb*
