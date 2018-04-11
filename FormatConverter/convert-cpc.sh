#!/bin/bash

FMTCAT=/opt/formatconverter/preventtools
FMTCNV=/opt/formatconverter/formatconverter
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/formatconverter

if [ $# -ne 2 ]; then
  echo "Syntax: $0 <conversion directory> <bed lookup file>"
  exit 1
fi

DIR=$1
BEDFILE=/mnt/ramdisk/bedfile

if [ ! -f $BEDFILE ]; then
  echo "copying $2 to $BEDFILE"
  cp $2 $BEDFILE
fi

PROCLOG=processing.log
ERRLOG=errors.log
DONELOG=done.log

ls $DIR | while read datedir; do
  ls $DIR/$datedir | while read bid; do
    PREFIX=$$
    echo "$DIR/$datedir/$bid"

    ls $DIR/$datedir/$bid/*.zip | while read zipfile; do 
      CHECKNAME=$bid/$(basename $zipfile)
      #echo "checking for string: $CHECKNAME"
      bed=$(LC_ALL=C fgrep $CHECKNAME $BEDFILE|cut -d\" -f4)

      #echo "  $zipfile"
      unzip -Z1 $zipfile | while read file; do
        unzip -p $zipfile $file | $FMTCNV --from cpcxml --to hdf5 --one-file --pattern /mnt/ramdisk/$file.hdf5 --quiet -
        mv /mnt/ramdisk/$file.hdf5 /mnt/ramdisk/${bed}+-${file}.hdf5
      done      
    done

    ls /mnt/ramdisk | grep -v bedfile | cut -d\+ -f1 | sort -u | while read bed; do 
      fname=/media/sf_home/Desktop/converted/${bed:=unknown}-${datedir}.hdf5
      echo "$FMTCAT --output ${fname} --cat $(ls /mnt/ramdisk/${bed}*.hdf5)"
      $FMTCAT --output ${fname} --cat $(ls /mnt/ramdisk/${bed}*.hdf5)
      rm  -rf /mnt/ramdisk/${bed}*
    done
  done
done
