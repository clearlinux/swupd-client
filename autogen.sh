#!/bin/sh

set -e

autoreconf --force --install --symlink --warnings=all

args="\
--sysconfdir=/etc \
--localstatedir=/var \
--prefix=/usr \
--enable-silent-rules"

./configure CFLAGS="-g -O2 $CFLAGS" $args "$@"
make clean
