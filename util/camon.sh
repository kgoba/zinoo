#!/usr/bin/env bash

#VIDEO_OPT='-b 1000000 -w 600 -h 480 -fps 24 -rot 180'
#VIDEO_OPT='-b 1500000 -w 800 -h 600 -fps 24 -rot 180'
#VIDEO_OPT='-b 1500000 -w 848 -h 480 -fps 24 -rot 180'
#VIDEO_OPT='-b 2000000 -w 1024 -h 576 -fps 24 -rot 180'
VIDEO_OPT='-b 3000000 -w 1280 -h 720 -fps 24 -rot 180'

BASE=/home/pi/capture

#STAMP=`date +%Y-%m-%d.%H:%M`
# Find a filename that is not yet taken
STAMP=1
while [[ -e $BASE-$STAMP.mpg ]] ; do
    let STAMP++
done

CAPTURE="$BASE-${STAMP}.mpg"
STREAM="#duplicate{dst=std{access=http,mux=ts,dst=:8554},dst=std{access=file,mux=ts,dst=$CAPTURE}}"
#STREAM='#std{access=http,mux=ts,dst=:8554}'

raspivid -o - -t 0 ${VIDEO_OPT} | cvlc -v stream:///dev/stdin --sout "${STREAM}" :demux=h264 :no-audio
