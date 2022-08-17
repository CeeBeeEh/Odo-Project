cmake_minimum_required(VERSION 3.15)
project(Odo)

add_subdirectory(gst/odo-detector)
add_subdirectory(gst/odo-osd)
add_subdirectory(gst-libs/odo-meta)
add_subdirectory(plugins/odo-lib-opencv-yolo)