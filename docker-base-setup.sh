#!/bin/bash

apt-get update
apt-get install -y \
	vim \
	curl \
	git \
	libsqlite3-dev \
	cmake \
	g++ \
	pkg-config \
	libhdf5-dev \
	libexpat-dev
	
# Matio has an ubuntu package, but we build it ourselves because of 
# inconsistencies in the packaging and CMAKE, and to avoid the dependency on HDF5 1.8 (we use 1.10)
curl --silent --create-dirs --output /tmp/matio.tgz https://ayera.dl.sourceforge.net/project/matio/matio/1.5.17/matio-1.5.17.tar.gz
cd /tmp
tar xfz matio.tgz
pushd matio-1.5.17
./configure --prefix=/usr
make
make install
popd

# WFDB doens't have an ubuntu package
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
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr CMakeLists.txt
make
make install
popd

ln -s /usr/lib/formatconverter-4.2.0/libformats.so /usr/lib
ln -s /usr/lib/tdmspp-osem-2.0.0/libtdmspp-osem.so /usr/lib
