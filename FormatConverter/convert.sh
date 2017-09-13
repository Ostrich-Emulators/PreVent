#!/bin/bash

STP2XML=/opt/StpToXml/StpToXml.exe
FMTCNV=/opt/formatconverter/formatconverter
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/formatconverter

function convert {
  STPFILE=$1
  OUTFMT=$2
  OUTDIR=$3
  ERRLOG=$4
  DONELOG=$5

  BASESTP=$(basename $STPFILE)
  STPSIZE=$(ls -s $STPFILE | cut -d\  -f1)
  TMPOUT=/mnt/ramdisk/xml.xml
  if [ $STPSIZE -gt 500000 ]; then
    TMPOUT=/tmp/xml.xml
  fi

  EXT="${STPFILE##*.}"
  echo $EXT
  if [ "$EXT" = "xml" ]; then
    TMPOUT=$STPFILE
  else
    mono $STP2XML $STPFILE -utc -o $TMPOUT

    if [ $? -ne 0 ]; then
      echo "$STPFILE stp error" >> $ERRLOG
      echo "error converting $STPFILE"
      return
    fi
  fi

  $FMTCNV --to $OUTFMT --prefix $BASESTP --outdir $OUTDIR $TMPOUT
  if [ $? -ne 0 ]; then
    echo "$STPFILE conversion error" >> $ERRLOG
    echo "error converting $STPFILE"
  else
    echo "$STPFILE" >> $DONELOG
  fi
}

###########################################################################
## This is the whole program...the function above does all the work!     ##
###########################################################################

if [ $# -ne 1 ]; then
  echo "Syntax: $0 <conversion directory>"
  exit 1
fi

DIR=$1

PROCLOG=$DIR/processing.log
ERRLOG=$DIR/errors.log
DONELOG=$DIR/done.log

touch PROCLOG
ls -tr $DIR/*.Stp $DIR/*.xml 1>/dev/null 2>&1
x=$?
if [ "$x" -eq "0" ] ; then
  FMT=$(echo $DIR|cut -d- -f2)
  OUTDIR=$DIR/converted
  mkdir -p $OUTDIR
  for STPFILE in $(ls -tr $DIR/*.Stp $DIR/*.xml); do
    cat $PROCLOG $DONELOG | grep -q $STPFILE
    x=$?
    if [ "$x" -ne "0" ] ; then
      echo "$STPFILE $FMT $OUTDIR $ERRLOG $DONELOG" >> $PROCLOG
    fi
  done
fi

cat $PROCLOG | while read STPFILE FMT OUTDIR ERRLOG DONELOG; do
  convert "$STPFILE" $FMT "$OUTDIR" "$ERRLOG" "$DONELOG"
  TMPPROC=/tmp/processing.$$
  grep -v "$STPFILE" "$PROCLOG" > $TMPPROC
  mv $TMPPROC "$PROCLOG"
done 
