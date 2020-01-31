FROM ubuntu:19.10
LABEL maintainer="Ryan Bobko <ryan@ostrich-emulators.com>"
LABEL BUILD_SIGNATURE="PreVent Format Converter"
LABEL version="4.2.0"

COPY docker-base-setup.sh /tmp
RUN /tmp/docker-base-setup.sh
#CMD ["/usr/bin/formatconverter", "--to", "hdf5"]