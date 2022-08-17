# The odolib_yolo Plugin
odolib_yolo_sources = [
  'odo-lib_opencv_yolo.cpp'
  ]

odolib_yolo = library('odo-lib-opencv-yolo',
  odolib_yolo_sources,
  c_args: plugin_c_args,
  dependencies : [opencv_dep],
  install : true,
  install_dir : plugins_install_dir,
)
