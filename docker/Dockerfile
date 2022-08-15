FROM ubuntu:22.04 AS BUILD_IMAGE

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


FROM ubuntu:22.04

RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y \
	libzip4 \
	sqlite3 \
	libhdf5-cpp-103 \
	expat

COPY --from=BUILD_IMAGE /usr/lib/*matio* /usr/lib/*wfdb* /usr/lib/tdms* /usr/lib/formatc* /usr/lib/
COPY --from=BUILD_IMAGE /usr/bin/formatc* /usr/bin/

ENTRYPOINT ["/usr/bin/formatconverter"]
CMD []
