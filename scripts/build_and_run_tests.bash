#!/bin/bash
# Simple script to build swupd with multiple configurations and run all needed tests with different configurations

set -e

#configurable parameters
if [ -z "$JOB_COUNT" ]; then
	JOB_COUNT=2
fi

CONFIGURE_ARGS="--with-fallback-capaths=./swupd_test_certificates -with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path=./testconfig"

export RUNNING_IN_CI=true
run_build() {
	make distclean || echo "No distclean - System already cleaned"
	autoreconf -fi
	# shellcheck disable=SC2068
	# We want to expand the variable, so ignore shellcheck warning
	./configure $@
	make V=1 -j "${JOB_COUNT}"
}

run_checks() {
	make -j "$JOB_COUNT" check
	make -j "$JOB_COUNT" functional-check
	make compliant
	make shellcheck
	make docs-coverage
	make distcheck
}

# Run tests without any security optimization and with address sanitizer
CFLAGS="-fsanitize=address -fno-omit-frame-pointer" run_build --disable-optimizations "$CONFIGURE_ARGS"
run_checks

# Run build with basic configurations
CFLAGS="$CFLAGS -O3 -falign-functions=32 -ffat-lto-objects -flto=4 -fno-math-errno -fno-semantic-interposition -fno-trapping-math" run_build "$CONFIGURE_ARGS"
./swupd -v |grep "+SIGVERIFY"
run_checks

## Make sure build isn't broken for other settings
run_build --disable-signature-verification --disable-third-party
./swupd -v |grep "\\-SIGVERIFY"
