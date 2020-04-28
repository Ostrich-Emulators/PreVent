FROM ubuntu:19.10
LABEL maintainer="Ryan Bobko <ryan@ostrich-emulators.com>", \
BUILD_SIGNATURE="PreVent Format Converter", \
version="4.2.2"

RUN apt-get update
RUN apt-get install -y \
	vim \
	curl \
	git \
	libsqlite3-dev \
	cmake \
	g++ \
	pkg-config \
	libhdf5-dev \
	libexpat-dev

COPY docker-base-setup.sh /tmp

RUN /tmp/docker-base-setup.sh
ENTRYPOINT ["/usr/bin/formatconverter"]
CMD []
