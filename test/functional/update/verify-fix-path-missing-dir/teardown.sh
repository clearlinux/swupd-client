#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/10/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/c2fb5e545287e632581b82e371de5d4aac2c7eb50d7f357f3b66fb0fdca71dd9
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') target-dir/usr 
sudo rm target-dir/usr/bin/foo
sudo rmdir target-dir/usr/bin/
