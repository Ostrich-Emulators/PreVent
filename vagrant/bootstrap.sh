#!/usr/bin/env bash

wget -q https://packages.microsoft.com/keys/microsoft.asc -O- | apt-key add -

add-apt-repository "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main"

apt-get update
#apt-get upgrade --assume-yes
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
	libsqlite3-dev \
	virtualbox-guest-dkms \
	virtualbox-guest-utils \
	virtualbox-guest-x11 \
	lightdm \
	lightdm-gtk-greeter \
	xfce4-whiskermenu-plugin \
	firefox \
	code \
	cmake \
	

	
sed -i -e 's/console/anybody/' /etc/X11/Xwrapper.config

VBoxClient --clipboard
VBoxClient --draganddrop
VBoxClient --display
VBoxClient --checkhostversion


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
git clone https://github.com/Ostrich-Emulators/TDMSpp.git
cd TDMSpp

cmake CMakeLists.txt
make 
sudo make install

git clone -b cmake https://github.com/Ostrich-Emulators/PreVent.git
cd PreVent
cmake CMakeLists.txt
make
sudo make install
EOF

cp /vagrant/file_conversion_check.sh /usr/local/bin
crontab -l > /tmp/crontab.root
echo "* * * * * /usr/local/bin/file_conversion_check.sh" >> /tmp/crontab.root 
cat /tmp/crontab.root | crontab -
