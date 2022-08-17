cmake_minimum_required(VERSION 3.15)
project(gst-odo-meta CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
find_library(GSTREAMER gstreamer-1.0)
find_library(GSTREAMER_VIDEO gstvideo-1.0)

find_library(GSTBASE gstbase-1.0)
find_library(GOBJECT gobject-2.0)
find_library(GLIB glib-2.0)


message("Gstreamer package ${GSTREAMER}")
message("Gstreamer video package ${GSTREAMER_VIDEO}")

set(SOURCE_FILES odometa.cpp odometa.h)

include_directories(.)

include_directories(/usr/include/gstreamer-1.0)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
#link_directories(/usr/lib/x86_64-linux-gnu/gstreamer-1.0)
include_directories(include)

add_library(gst-odo-meta SHARED ${SOURCE_FILES})

target_link_libraries(gst-odo-meta ${GSTREAMER} ${GSTREAMER_VIDEO} ${CUDALIB} ${GSTBASE} ${GOBJECT} ${GLIB})
target_link_libraries(gst-odo-meta m gstrtspserver-1.0)
#target_link_libraries(gst-odo-detector "libOpenCV_Yolo.so")
