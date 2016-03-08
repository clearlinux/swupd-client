#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') target-dir/usr/lib/kernel/testfile

rm -f web-dir/*/*.tar
