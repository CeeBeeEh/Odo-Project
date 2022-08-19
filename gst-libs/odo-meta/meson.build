#project('odo-meta', 'cpp', version : '1.19.0.1', license : 'LGPL')

#plugins_install_dir = '/mnt/4TB/Development/Odo/'

gst_version = meson.project_version()

api_version = '1.0'

plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'odo-meta-shared-library')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'Odo-Meta Gst Shared Library')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)

# The gst-odo-meta shared library
gstodometa_sources = [
  'odometa.cpp'
  ]

gstodometa = library('odo-meta',
  gstodometa_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep, gst_video_dep, gstbase_dep],
  install : true,
  install_dir : lib_install_dir,
)
