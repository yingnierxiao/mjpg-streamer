#!/bin/sh

export LD_LIBRARY_PATH="$(pwd)"

./mjpg_streamer -i "./input_uvc.so -d /dev/video0 -m 2048  -r 320x240 -f 10  -yuv -n" -o "./output_http_push.so -p 5001 -s dp.io -f 30 -a ?oid=rpi"



