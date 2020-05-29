#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Syntax: $0 <file 1> <file 2>"
  exit 1
fi

F1=$1
F2=$2
PREVENT=PreVentTools/preventtools

CHECKWAVES=true

echo "Vitals:"
$PREVENT $F1 --vitals | cut -c13- | while read signal ; do
  printf "  %12s" "$signal..."
  SIG1=$(echo "/tmp/$signal-v-one.txt" | tr [:space:] '_' )
  SIG2=$(echo "/tmp/$signal-v-two.txt" | tr [:space:] '_' )
    
  rm -rf $SIG1 $SIG2
  $PREVENT $F1 --print --path "/VitalSigns/$signal" --output $SIG1 
  $PREVENT $F2 --print --path "/VitalSigns/$signal" --output $SIG2
  if [ $? -ne 0 ]; then
    echo "$F2 is missing $signal!"
  else
    MD1=$(md5sum $SIG1 | cut -d\  -f1)
    MD2=$(md5sum $SIG2 | cut -d\  -f1)
    
    if [ $MD1 = $MD2 ]; then
      echo "identical"
    else 
      echo "DIFFERENT ($SIG1 $SIG2)"
      CHECKWAVES=false
    fi
  fi
done

if [ $CHECKWAVES ] ; then
  echo "Waves:" 
  $PREVENT $F1 --waves | cut -c12- | while read signal ; do
    printf "  %10s" "$signal..."
  SIG1=$(echo "/tmp/$signal-w-one.txt" | tr [:space:] '_' )
  SIG2=$(echo "/tmp/$signal-w-two.txt" | tr [:space:] '_' )
  
    rm -rf $SIG1 $SIG2
    $PREVENT $F1 --print --path "/Waveforms/$signal" --output $SIG1 
    $PREVENT $F2 --print --path "/Waveforms/$signal" --output $SIG2
    if [ $? -ne 0 ]; then
      echo "$F2 is missing $signal!"
    else
      MD1=$(md5sum $SIG1 | cut -d\  -f1)
      MD2=$(md5sum $SIG2 | cut -d\  -f1)
   
      if [ $MD1 = $MD2 ]; then
        echo "identical"
      else 
        echo "DIFFERENT ($SIG1 $SIG2)"
      fi
    fi
  done
else
  echo "skipping waves because not all vitals passed"
fi
