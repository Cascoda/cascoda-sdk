#!/bin/bash
# Usage: aerial-adapter channel_nr [serial_nr]

if [ $# -lt 1 ]; then
 echo "Usage: aerial-adapter channel_nr [serial_nr]"
 exit
fi

channel=$1

if [ $channel -lt 11 ]; then
 echo "Channel Nr invalid"
 exit
elif [ $channel -gt 26 ]; then
 echo "Channel Nr invalid"
 exit
fi

if [ $# == 2 ]; then

serialno=$2

 # MLME-SET-Request phyCurrentChannel - with serialno
 printf 4A04000001%02X $channel | xxd -r -p | chilictl.exe pipe -s $serialno | xxd -p
 # start serial-adapter - with serialno
 serial-adapter $serialno
 #chilictl.exe pipe -s $serialno

else

 # MLME-SET-Request phyCurrentChannel - without serialno
 printf 4A04000001%02X $channel | xxd -r -p | chilictl.exe pipe | xxd -p
 # start serial-adapter - without serialno
 serial-adapter
 #chilictl.exe pipe

fi
