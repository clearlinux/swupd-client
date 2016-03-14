#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3
sudo rmdir target-dir/usr/bin/
sudo rm target-dir/usr/foo
