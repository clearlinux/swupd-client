#!/bin/bash

set -e

os_release_file=1c1efd10467cbc1dddd8e63c3ef4d9099f36418ed5372d080a1bf0f03a49ab05
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/30/staged/$os_release_file
mv $PWD/testfile target-dir/
sed -i 's/30/20/' target-dir/usr/lib/os-release
