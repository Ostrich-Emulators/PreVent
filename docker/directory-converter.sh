#!/bin/bash

cd /conversion

# first things first: there are three conversion types that use a directory structure:
# 1) WFDB -- contains a .hea file
# 2) DWC  -- contains a .info file
# 3) UVA  -- contains .gzip files
# Check any subdirectories in /conversion to see if we have one of those types 
for dir in $(ls -d */); do
	echo "converting directory: $dir"
	if [ $(ls ${dir}*.info 2>/dev/null|wc -l) -gt 0 ]; then
	  cd $dir
	  echo $dir is a DWC dataset
	  for info in $(ls *.info); do
		  formatconverter --to hdf5 --from dwc --localtime $info
	  done
	  cd ..
	elif [ $(ls ${dir}*.hea 2>/dev/null|wc -l) -gt 0 ]; then
	  echo $dir is a WDFB dataset
	  cd $dir
	  for hea in $(ls *.hea); do
		  formatconverter --to hdf5 --from wfdb --localtime $hea
	  done
	  cd ..
	elif [ $(ls ${dir}*.gzip 2>/dev/null| wc -l) -gt 0 ]; then
	  echo $dir is a UVA dataset
	  formatconverter --to hdf5 --from zl --localtime $dir
	fi 
done 

# now that we've converted any directories, do the files
for file in $(ls -p|grep -v /|grep -v .hdf5); do
	echo "converting file: $file"
	formatconverter --to hdf5 --localtime $file
done
