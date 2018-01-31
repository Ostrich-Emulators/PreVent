#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Syntax: $0 <workfile>"
  exit 1
fi

STP2XML=StpToXml/StpToXml.exe
TMPXML=/cygdrive/c/Windows/temp/xml.xml
OUTDIR=/cygdrive/c/Users/rpb6eg/Desktop/converted

ERRS=errors.log
DONE=done.log
CURR=current.log

touch $ERRS $DONE $CURR
set -o pipefail

cat $1 | while read line; do 
  grep -q "$line" $DONE
  if [ $? -eq 0 ]; then
    echo "skipping $line (already done)"
    continue
  fi
  grep -q "$line" $ERRS
  if [ $? -eq 0 ]; then
    echo "skipping $line (previously failed)"
    continue
  fi

  echo "$line" > $CURR
  echo "$STP2XML $(cygpath -w $line) -utc -o $(cygpath -w $TMPXML)"
  $STP2XML $(cygpath -w $line) -utc -o $(cygpath -w $TMPXML)
  if [ $? -ne 0 ]; then
    echo "stp error: $line" >> $ERRS
    echo "STP conversion error on $line"
    continue
  fi

  echo "./formatconverter.exe --to hdf5 $TMPXML --sqlite $OUTDIR/database.db --outdir $OUTDIR"
  ./formatconverter.exe --outdir $OUTDIR \
	--to hdf5 $TMPXML \
	--sqlite $OUTDIR/database.db \
	--prefix $(basename $line) 2>&1 | tee output.log
  if [ $? -ne 0 ]; then
    echo "formatconverter error: $line" >> $ERRS
    echo "format converter error on $line"
    continue
  fi

  grep -q "refusing" output.log
  if [ $? -eq 0 ]; then
    echo "$line (but missing data)" >> $DONE
    echo "no data error: $line" >> $ERRS
    continue
  else
    echo "$line" >> $DONE
  fi

  rm -f output.log
done
