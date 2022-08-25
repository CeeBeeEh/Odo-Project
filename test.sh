#!/bin/bash

#####################################
##### User configured variables #####
ENABLE_REMOTE_GDB_DEBUGGER=0
TRACKER_ENABLED=0
GST_DEBUG_LEVEL=2
VIDEO="video.264"
TRACKER_TYPE="TLD"
INTERVAL=15
ENABLE_CUDA=1
CONFIDENCE=0.45
NMS=0.5
WIDTH=416
HEIGHT=416
CLASSES="$PWD/classes.txt"
CONFIG="$PWD/yolov4.cfg"
WEIGHTS="$PWD/yolov4.weights"
#####################################
#####################################

if [ -z "$1" ]
then
  printf "\nNo video provided as an argument, trying \"%s\"\n\n" "$VIDEO"
  sleep 1
else
  VIDEO=$!
fi

ODO_GST_PATH="$PWD/release/gstreamer/"
LIB_PATH="$PWD/release/"
TRACKER=""
GDBSERVER=""

if [ $ENABLE_REMOTE_GDB_DEBUGGER -eq 1 ]; then
  GDBSERVER="gdbserver 127.0.0.1:1234"
fi

if [ $TRACKER_ENABLED -eq 1 ]; then
  TRACKER="odo_track type=$TRACKER_TYPE !"
else
  INTERVAL=0
fi

if [ ! -f "$CLASSES" ]; then
  printf "\nYolo classes file not found=%s" "$CLASSES"
  printf "\nTry downloading them with \"get_yolo.sh\"\n\n"
  exit 1
fi
if [ ! -f "$CONFIG" ]; then
  printf "\nYolo config file not found=%s" "$CONFIG"
  printf "\nTry downloading them with \"get_yolo.sh\"\n\n"
  exit 1
fi
if [ ! -f "$WEIGHTS" ]; then
  printf "\nYolo weights file not found=%s" "$WEIGHTS"
  printf "\nDownload yolo files with \"get_yolo.sh\"\n\n"
  exit 1
fi

export LD_LIBRARY_PATH=$LIB_PATH:$LD_LIBRARY_PATH
export GST_DEBUG=$GST_DEBUG_LEVEL
export GST_PLUGIN_PATH=$ODO_GST_PATH:$GST_PLUGIN_PATH

$GDBSERVER /usr/bin/gst-launch-1.0 filesrc location=$VIDEO \
! h264parse ! avdec_h264 ! videoconvert ! odo_detector interval=$INTERVAL cuda=$ENABLE_CUDA \
confidence=$CONFIDENCE nms=$NMS width=$WIDTH height=$HEIGHT lib_path=$LIB_PATH/libodo-lib-opencv-yolo.so \
net_classes=$CLASSES net_config=$CONFIG net_weights=$WEIGHTS ! $TRACKER odo_viz ! videoconvert ! autovideosink
