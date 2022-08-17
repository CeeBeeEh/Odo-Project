#!/bin/bash

rm ~/.cache/gstreamer-1.0/registry.x86_64.bin
rm builddir/ -rf 
meson builddir 
ninja -C builddir 
sudo ninja -C builddir install
