#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo rmdir target-dir/usr/bin/
