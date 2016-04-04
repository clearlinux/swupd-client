#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/10/files/491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/10/files/826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/10/files/a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50
sudo rm target-dir/os-core
sudo rm target-dir/test-bundle1
sudo rm target-dir/test-bundle2
