#!/bin/bash

VIDEO="video.264"

if [ -z "$1" ]
then
  printf "\nNo video provided as an argument, trying \"%s\"\n\n" "$VIDEO"
  sleep 3
else
  VIDEO=$!
fi

ODO_GST_PATH="$PWD/release/gstreamer/"
LIB_PATH="$PWD/release/"


ENABLE_CUDA=1
CONFIDENCE=0.45
NMS=0.5
WIDTH=416
HEIGHT=416
CLASSES="$PWD/classes.txt"
CONFIG="$PWD/yolov4.cfg"
WEIGHTS="$PWD/yolov4.weights"

if [ ! -f "$CLASSES" ]
then
  printf "\nYolo classes file not found=%s" "$CLASSES"
  printf "\nTry downloading them with \"get_yolo.sh\"\n\n"
  exit 1
fi
if [ ! -f "$CONFIG" ]
then
  printf "\nYolo config file not found=%s" "$CONFIG"
  printf "\nTry downloading them with \"get_yolo.sh\"\n\n"
  exit 1
fi
if [ ! -f "$WEIGHTS" ]
then
  printf "\nYolo weights file not found=%s" "$WEIGHTS"
  printf "\nDownload yolo files with \"get_yolo.sh\"\n\n"
  exit 1
fi


LD_LIBRARY_PATH=$LIB_PATH:$LD_LIBRARY_PATH GST_PLUGIN_PATH=$ODO_GST_PATH gst-launch-1.0 filesrc location=$VIDEO \
! h264parse ! avdec_h264 ! videoconvert ! odo_detector cuda=$ENABLE_CUDA confidence=$CONFIDENCE nms=$NMS \
width=$WIDTH height=$HEIGHT lib_path=$LIB_PATH/libodo-lib-opencv-yolo.so net_classes=$CLASSES net_config=$CONFIG \
net_weights=$WEIGHTS ! odo_viz ! videoconvert ! autovideosink
