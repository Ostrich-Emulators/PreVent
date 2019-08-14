#!/bin/bash

#export LISTENLOCATION="/vagrant/to-convert"
export LISTENLOCATION="to-convert"

WORK=$LISTENLOCATION/.working # file we're currently working on
DONE=$LISTENLOCATION/.done    # files we've already converted
TODO=$LISTENLOCATION/.todo    # files we haven't converted yet
THREADS=$LISTENLOCATION/threads # desired number of files to convert at once
declare -i threadcount=1;

touch $WORK $DONE $TODO
ls $LISTENLOCATION/*.xml > /tmp/todo

if [ -s $THREADS ]; then
  threadcount=$(cat $THREADS)
fi

# ignore any files we've already seen
for file in $WORK $DONE $TODO; do 
  grep -v -f $file /tmp/todo > /tmp/new-work
  mv /tmp/new-work /tmp/todo
done

# remember for next time what we're working on this time
cat /tmp/todo >> $TODO

# if we're working $threadcount files (or more, if
# threadcount has changed since last run), we're done
declare -i inprocess=$(wc -l $WORK | cut -d\  -f1)
if [ $inprocess -lt $threadcount ]; then
  if [ -s $TODO ]; then

    # get the first file from the todo file
    input=$(head -1 $TODO)
    tail +2 $TODO > /tmp/todo
    mv /tmp/todo $TODO
    echo $input >> $WORK

    echo "now converting $input"
    /usr/local/bin/formatconverter --to hdf5 $input --no-break --pattern %d/%S
    rslt=$?
    if [ $rslt -eq 0 ]; then
      # successfully converted the file, so move it from $WORK to $DONE
      grep -v $input $WORK > /tmp/work
      mv /tmp/work $WORK
      echo "$input" >> $DONE
    else
      # put the file back on the todo pile
      echo "$input" >> $TODO
    fi
  fi
fi
