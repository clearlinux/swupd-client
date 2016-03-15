#!/bin/bash

set -e

sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/d56e570097c2932ff0454cb45ef37964c6a0792e0224013f9e1d837cc22aa9fa
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/95a84952523cf67d92b9f4f03057ca1bf6e1ff018104f7f7ad27d5d4fb38b9ad
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/82206f761ee900b5227656f443f9ac4beb32c25573da6b59bf597e894f4eceac
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/6e5658ceb83e69365ded0684df8f38bcc632f71780ae10ff19b87616a03d6337
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/7726ea82e871eb80c9a72ef95e83ae00e2e8097e7d99ce43b8e47afb1113414b
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/34c8113c07c33167de84e9b9c5557895c958aece3519669706a63a0dafee2481
sudo chown $(ls -l setup.sh | awk '{ print $3 ":" $4 }') web-dir/100/files/39e14cebdd7f0cab5089a9cecf97f5e77b226d00325e22a0a9081eb544e0b6f0
sudo rmdir target-dir/usr/bin/
sudo rm target-dir/usr/core
sudo rm target-dir/usr/foo2
sudo rm target-dir/usr/foo3
sudo rm target-dir/usr/foo4
sudo rm target-dir/usr/foo5
sudo rm target-dir/usr/foo6
sudo rm target-dir/usr/foo7
