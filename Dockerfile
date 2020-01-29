FROM ubuntu:19.10

RUN mkdir -p /opt/fmtcnv
COPY docker-base-setup.sh /opt/fmtcnv
WORKDIR /opt/fmtcnv