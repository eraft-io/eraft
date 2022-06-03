#!/bin/bash
ret=`ps aux | grep -v grep | grep "emon -i /home/dennis/emon/skx-2s-events.txt"|wc -l`
if [ $ret == 0 ]; then
/home/dennis/emon/sepdk/src/rmmod-sep
/home/dennis/emon/sepdk/src/insmod-sep
echo "/home/dennis/emon/bin64/emon -i /home/dennis/emon/skx-2s-events.txt > $1 &"
/home/dennis/emon/bin64/emon -i /home/dennis/emon/skx-2s-events.txt > $1 &
fi
