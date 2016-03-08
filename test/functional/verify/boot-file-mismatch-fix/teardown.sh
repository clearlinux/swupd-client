#!/bin/bash

set -e

targetfile=target-dir/usr/lib/kernel/testfile

sudo cp $targetfile.backup $targetfile
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') $targetfile

rm -f web-dir/*/*.tar
rm -f web-dir/*/files/*.tar
