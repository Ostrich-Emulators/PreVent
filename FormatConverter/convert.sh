#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Syntax: $0 <path to StpToXml.exe> <input directory> <output directory>"
  exit 1
fi

STP2XML=$1
INPUTDIR=$2
OUTPUTDIR=$3
FMTCNV=dist/Release/GNU-Linux/formatconverter

function convert {
  STPFILE=$1
  BASESTP=$(basename $STPFILE)
  STPSIZE=$(ls -s $STPFILE | cut -d\  -f1)
  TMPOUT=/mnt/ramdisk/xml.xml
  if [ $STPSIZE -gt 500000 ]; then
    TMPOUT=/tmp/xml.xml
  fi

  mono $STP2XML $STPFILE -utc -o $TMPOUT
  if [ $? -ne 0 ]; then
    touch $OUTDIR/${BASESTP}-stp-error
    echo "error converting $STPFILE"
  fi

  $FMTCNV --to hdf5 $TMPOUT --prefix $BASESTP --outdir $OUTPUTDIR
  if [ $? -ne 0 ]; then
    touch $OUTDIR/${BASESTP}-conv-error
    echo "error converting $STPFILE"
  fi
}

###########################################################################
## This is the whole program...the function above does all the work!     ##
###########################################################################
if [ -f $INPUTDIR ]; then
  convert $INPUTDIR
else
  for STPFILE in $(ls $INPUTDIR/*.Stp); do
    convert $STPFILE   
  done
fi


