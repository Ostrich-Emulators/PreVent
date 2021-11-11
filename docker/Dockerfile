FROM ubuntu:20.04
LABEL maintainer="Ryan Bobko <ryan@ostrich-emulators.com>", \
BUILD_SIGNATURE="PreVent Format Converter", \
version="XXVERSIONXX"

RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y \
	vim \
	curl \
	git \
	libzip-dev \
	libsqlite3-dev \
	cmake \
	g++ \
	libtool \
	m4 \
	automake \
	pkg-config \
	libhdf5-dev \
	libexpat-dev

COPY .docker-base-setup-XXVERSIONXX.sh /tmp/docker-base-setup.sh

RUN /tmp/docker-base-setup.sh XXVERSIONXX
ENTRYPOINT ["/usr/bin/formatconverter"]
CMD []
