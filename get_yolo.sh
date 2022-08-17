#!/bin/bash

# https://github.com/AlexeyAB/darknet/wiki/YOLOv4-model-zoo

# Classes file
printf "\n\n\n============== Downloading classes list file ==============\n\n"
wget https://raw.githubusercontent.com/AlexeyAB/darknet/master/data/coco.names -O classes.txt

# Yolov7
printf "\n\n\n============== Downloading Yolov7 config and weights files ==============\n\n"
wget https://github.com/WongKinYiu/yolov7/releases/download/v0.1/yolov7.weights
wget https://raw.githubusercontent.com/WongKinYiu/yolov7/darknet/cfg/yolov7.cfg

# Yolov4
printf "\n\n\n============== Downloading Yolov4 config and weights files ==============\n\n"
wget https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4.cfg 
wget https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v3_optimal/yolov4.weights


# Yolov4-tiny
printf "\n\n\n============== Downloading Yolov4-tiny config and weights files ==============\n\n"
wget https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4-tiny.cfg
wget https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v4_pre/yolov4-tiny.weights
