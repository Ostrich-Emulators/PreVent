#!/usr/bin/env bash

apt-get update
apt-get --assume-yes install \
	libhdf5-dev \
	gcc g++\
	libcurl4-openssl-dev \
	libexpat1-dev \
	default-jdk \
	libmatio-dev \
	pkg-config \
	git \
	xfce4 \
	default-jdk \
	libsqlite3-dev
	

wget -q https://www.physionet.org/physiotools/wfdb.tar.gz -O /tmp/wfdb.tgz

pushd /tmp
tar zxvf /tmp/wfdb.tgz
popd 
pushd /tmp/wfdb-10.6.2
./configure -i <<EOF



EOF
make
make install
popd

su - vagrant <<EOF
git clone https://github.com/Ostrich-Emulators/PreVent.git
cd PreVent/FormatConverter
make
EOF

# todo: netbeans

