plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-track-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'Odo-Track Gst Plug-in')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://github.com')
configure_file(output : 'config.h', configuration : cdata)

# The gstododetector Plugin
gstodotrack_sources = [
  'gstodotrack.cpp'
  ]

gstodotrack = library('gstodotrack',
  gstodotrack_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep, gst_video_dep, gstbase_dep, opencv_dep, odo_meta_dep],
  install : true,
  install_dir : plugins_install_dir,
)
