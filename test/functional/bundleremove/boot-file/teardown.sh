#!/bin/bash

set -e

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

sudo rm -f target-dir/usr/lib/kernel/testfile
cp web-dir/10/staged/$targetfile target-dir/usr/lib/kernel/testfile
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') target-dir/usr/lib/kernel/testfile
touch target-dir/usr/share/clear/bundles/test-bundle

rm -f web-dir/*/*.tar
