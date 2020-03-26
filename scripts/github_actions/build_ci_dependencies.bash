#!/bin/bash
# Build all custom dependencies needed by swupd:
# - Curl
# - Bsdiff
# - libarchive
#
# As process is slow, it can be cached. If directory dependencies exists,
# just install already build dependencies.

set -e
CORES=2

# If alread exists, reuse
if [ -d dependencies ]; then
	pushd dependencies
	for i in *; do
		if [ -d "$i" ]; then
			pushd "$i"
			sudo make install
			popd
		fi
	done

	popd
	exit
fi

mkdir dependencies
pushd dependencies

# build bsdiff
wget https://github.com/clearlinux/bsdiff/releases/download/v1.0.2/bsdiff-1.0.2.tar.xz
tar -xvf bsdiff-1.0.2.tar.xz
pushd bsdiff-1.0.2 && ./configure --prefix=/usr --disable-tests && make -j"$CORES" && sudo make install && popd

## build curl
wget https://curl.haxx.se/download/curl-7.64.0.tar.gz
tar -xvf curl-7.64.0.tar.gz
pushd curl-7.64.0 && ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu && make -j"$CORES" && sudo make install && popd

# build libarchive
wget https://github.com/libarchive/libarchive/archive/v3.3.1.tar.gz
tar -xvf v3.3.1.tar.gz
pushd libarchive-3.3.1 && autoreconf -fi && ./configure --prefix=/usr && make -j"$CORES" && sudo make install && popd

popd
