#!/bin/bash

set -e

deltafile="10-100-9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a"
origfile="0832c0c7fb9d05ac3088a493f430e81044a0fc03f81cba9eb89d3b7816ef3962"

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/delta/$deltafile

# replace the original file with the copy in staged/
sudo rm target-dir/testfile
cp web-dir/10/staged/$origfile target-dir/testfile
rmdir web-dir/100/staged
