#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Syntax: $0 <workfile>"
  exit 1
fi

STP2XML=StpToXml/StpToXml.exe
TMPXML=/cygdrive/c/Windows/temp/xml.xml
OUTDIR=/cygdrive/c/Users/rpb6eg/Desktop/converted

cat $1 | while read line; do 
  grep -q "$line" done.log
  if [ $? -eq 0 ]; then
    echo "skipping $line (already done)"
    continue
  fi
  grep -q "$line" errors.log
  if [ $? -eq 0 ]; then
    echo "skipping $line (previously failed)"
    continue
  fi

  echo "$line" > current.log
  echo "$STP2XML $(cygpath -w $line) -utc -o $(cygpath -w $TMPXML)"
  $STP2XML $(cygpath -w $line) -utc -o $(cygpath -w $TMPXML)
  if [ $? -ne 0 ]; then
    echo "stp error: $line" >> errors.log
    echo "STP conversion error on $line"
    continue
  fi

  echo "./formatconverter.exe --to hdf5 $TMPXML --sqlite $OUTDIR/database.db --outdir $OUTDIR"
  ./formatconverter.exe --outdir $OUTDIR \
	--to hdf5 $TMPXML \
	--sqlite $OUTDIR/database.db \
	--prefix $(basename $line)
  if [ $? -ne 0 ]; then
    echo "formatconverter error: $line" >> errors.log
    echo "format converter error on $line"
    continue
  fi
  echo "$line" >> done.log
done
