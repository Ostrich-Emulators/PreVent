#!/bin/bash

FMTCAT=/opt/formatconverter/preventtools
FMTCNV=/opt/formatconverter/formatconverter
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/formatconverter

if [ $# -ne 1 ]; then
  echo "Syntax: $0 <conversion directory>"
  exit 1
fi

DIR=$1

PROCLOG=processing.log
ERRLOG=errors.log
DONELOG=done.log

ls $DIR | while read datedir; do
  ls $DIR/$datedir | while read bid; do
    ls $DIR/$datedir/$bid/*.zip | while read zipfile; do 
      PREFIX=$$
      unzip -Z1 $zipfile | while read file; do
        unzip -p $zipfile $file | $FMTCNV --from cpcxml --to hdf5 --one-file --quiet -
        mv ./- ${PREFIX}-${file}.hdf5
      done
      fname=${datedir}-${bid}.hdf5
echo "$FMTCAT --output ${fname} --cat $(ls ${PREFIX}*.hdf5)"
      $FMTCAT --output ${fname} --cat $(ls ${PREFIX}*.hdf5)
      #rm -rf ${PREFIX}*
    done
  done
done
