#!/bin/sh

set -e

autoreconf --force --install --symlink --warnings=all

# The client certificate tests are skipped by default, but require an
# alternative trust store for the test web server's public key. To enable
# the trust store for the client certificate tests, the following configuration
# argument must be added to args:
# --with-fallback-capaths=/tmp/swupd_test_certificates
args="\
--sysconfdir=/etc \
--localstatedir=/var \
--prefix=/usr \
--enable-silent-rules"

./configure CFLAGS="-g -O2 $CFLAGS" $args "$@"
make clean
