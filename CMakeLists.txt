cmake_minimum_required(VERSION 3.15)
project(Odo)

add_subdirectory(gst/odo-detector)
add_subdirectory(gst/odo-viz)
add_subdirectory(gst/odo-track)
add_subdirectory(gst-libs/odo-meta)
add_subdirectory(detection-libs/odo-lib-opencv-yolo)