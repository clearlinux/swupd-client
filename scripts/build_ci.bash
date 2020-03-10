#!/bin/bash
set -e
CORES=2

# Install dependencies from ubuntu
sudo apt-get install python3-docutils
sudo apt-get install check
sudo apt-get install bats
sudo ln -s /bin/systemctl /usr/bin/systemctl

# Use rst2man from python3
sudo ln -s /usr/share/docutils/scripts/python3/rst2man /usr/bin/rst2man.py

# build bsdiff
wget https://github.com/clearlinux/bsdiff/releases/download/v1.0.2/bsdiff-1.0.2.tar.xz
tar -xvf bsdiff-1.0.2.tar.xz
pushd bsdiff-1.0.2 && ./configure --prefix=/usr --disable-tests && make -j$CORES && sudo make install && popd

## build curl
wget https://curl.haxx.se/download/curl-7.64.0.tar.gz
tar -xvf curl-7.64.0.tar.gz
pushd curl-7.64.0 && ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu && make -j$CORES && sudo make install && popd

# build libarchive
wget https://github.com/libarchive/libarchive/archive/v3.3.1.tar.gz
tar -xvf v3.3.1.tar.gz
pushd libarchive-3.3.1 && autoreconf -fi && ./configure --prefix=/usr && make -j$CORES && sudo make install && popd

# Build Swupd
autoreconf --verbose --warnings=none --install --force
./configure CFLAGS="$CFLAGS -fsanitize=address -Werror" --prefix=/usr --with-fallback-capaths=./swupd_test_certificates --with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path=./testconfig
make -j$CORES

# Needed to initialize the host for auto-update. Without these steps auto-update
# within a --path would just not work
sudo mkdir -p /usr/lib/systemd/system/
sudo cp data/swupd-update.service /usr/lib/systemd/system/
sudo cp data/swupd-update.timer /usr/lib/systemd/system/
sudo systemctl enable swupd-update.timer
sudo systemctl restart swupd-update.timer
