project('gst-odo', 'cpp', version : '1.19.0.1', license : 'LGPL')

plugins_install_dir = join_paths(meson.current_source_dir(), 'release/gstreamer')
lib_install_dir = join_paths(meson.current_source_dir(), 'release')

cc = meson.get_compiler('cpp')

gst_version = meson.project_version()

api_version = '1.0'

opencv_dep = dependency('opencv4', version : '>=4.5',
    required : true, fallback : ['opencv4'])
gst_dep = dependency('gstreamer-1.0', version : '>=1.19',
    required : true, fallback : ['gstreamer', 'gst_dep'])
gst_video_dep = dependency('gstreamer-video-1.0', version : '>=1.19',
    required : true, fallback : ['gstreamer', 'gst_video_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.19',
  fallback : ['gstreamer', 'gst_base_dep'])

# build odo-meta first
subdir('gst-libs/odo-meta')

odo_meta_dep = declare_dependency(link_with : gstodometa)
#odometa_dep = cc.find_library('odo-meta', dirs : plugins_install_dir)

subdir('gst/odo-detector')
subdir('gst/odo-viz')

subdir('detection-libs/odo-lib-opencv-yolo')
