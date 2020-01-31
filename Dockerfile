FROM ubuntu:18.04
LABEL maintainer="Ryan Bobko <ryan@ostrich-emulators.com>", \
BUILD_SIGNATURE="PreVent Format Converter", \
version="4.2.0"

COPY docker-base-setup.sh /tmp
RUN /tmp/docker-base-setup.sh
ENTRYPOINT ["/usr/bin/formatconverter"]
CMD []