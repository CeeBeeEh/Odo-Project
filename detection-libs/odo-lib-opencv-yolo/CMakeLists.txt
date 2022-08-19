cmake_minimum_required(VERSION 3.22)
project(OdoLib_OpenCV_Yolo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -W")

find_package(CUDA 11.7 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(".")

set(SOURCES odo-lib_opencv_yolo.cpp odo-lib_opencv_yolo.h)

add_library(OdoLib_OpenCV_Yolo SHARED ${SOURCES})

target_link_libraries(OdoLib_OpenCV_Yolo ${OpenCV_LIBS})
target_link_libraries(OdoLib_OpenCV_Yolo ${GLIB_LDFLAGS})
#target_link_libraries( OdoLib_OpenCV_Yolo PkgConfig::deps )
