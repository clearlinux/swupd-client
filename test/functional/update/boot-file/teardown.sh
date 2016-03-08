#!/bin/bash

set -e

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/$targetfile
sudo rm target-dir/usr/lib/kernel/testfile

rm -f web-dir/*/*.tar
rm -f web-dir/*/files/*.tar
