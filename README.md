# Odo-Project
Gstreamer based video surveillance application.

This is still in the alpha stage. Currently, all that it does is play a local video file, capture the inference\
data as a custom GstMeta object (OdoMeta), and render the results in an OSD window.

Meson is suggested for compilation, but gcc/cmake should work also. However, this has not been tested.

OpenCV 4.0 or newer is required. Building OpenCV with CUDA is required for GPU inferencing.

To build the components simply execute ./build.sh

To test, run ./test.sh

The variables in test.sh can be edited for customized results.
