#!/bin/bash
set -e
CORES=2

# Install dependencies from ubuntu
sudo apt-get install python3-docutils
sudo apt-get install shellcheck
sudo apt-get install doxygen
sudo apt-get install check
sudo apt-get install bats
sudo pip install coverxygen
sudo ln -s /bin/systemctl /usr/bin/systemctl

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

# Use rst2man from python3
sudo ln -s /usr/share/docutils/scripts/python3/rst2man /usr/bin/rst2man.py

# Install new clang-format from llvm repository
wget -O llvm.gpg.key https://apt.llvm.org/llvm-snapshot.gpg.key
sudo apt-key add llvm.gpg.key
sudo add-apt-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main"
sudo apt-get update
sudo apt-get install -y clang-format-9

# Build Swupd
autoreconf --verbose --warnings=none --install --force
./configure CFLAGS="$CFLAGS -fsanitize=address -Werror" --prefix=/usr --enable-third-party --with-fallback-capaths=./swupd_test_certificates --with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path=./testconfig
make -j$CORES
