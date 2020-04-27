#!/bin/bash
# Simple script to build swupd with multiple configurations and run all needed tests with different configurations

set -e

usage() {
	cat <<-EOM
		Usage:
		   ./scripts/build_and_run_tests.bash [-b] [-j <jobs>]

		Options:
		    -b    Don't run any tests, just build swupd with different configuration options
		    -j    Set the number of jobs used in a build and number of tests to run in parallel
	EOM
}

run_build() {
	make distclean || echo "No distclean - System already cleaned"
	autoreconf -fi
	# shellcheck disable=SC2068
	# We want to expand the variable, so ignore shellcheck warning
	./configure $@
	make V=1 -j "$JOB_COUNT"
}

run_checks() {
	if [ -n "$BUILD_ONLY" ]; then
		return
	fi

	make -j "$JOB_COUNT" check
	make -j "$JOB_COUNT" functional-check
	make compliant
	make docs-coverage
	make distcheck
}

run_checks_tartar() {
	if [ -n "$BUILD_ONLY" ]; then
		return
	fi

	# Tar tar doasnt support hardlinks
	# There's a known bug in tar tar code that doesn't support unusual file names
	FILES="$(find test/functional  -name "*.bats" \( ! -path "*only_in_ci_system*" \) | sort | grep -v install-hardlink-symlink.bats)"

	env TESTS="$FILES"  make -j "$JOB_COUNT" -e check
	env TESTS="$(find ./test/functional/only_in_ci_system  -name "*.bats" | sort)" make -e -j1 check

}

main() {
	# Parse parameters
	while getopts :bj: opt; do
		case "$opt" in
			b)	BUILD_ONLY=1 ;;
			j)	JOB_COUNT="$OPTARG" ;;
			*)	usage
				return ;;
		esac
	done
	shift $((OPTIND-1))

	if [ -z "$JOB_COUNT" ]; then
		JOB_COUNT=2
	fi

	export RUNNING_IN_CI=true
	export JOB_COUNT
	export BUILD_ONLY
	local configure_args="--with-fallback-capaths=""$PWD""/swupd_test_certificates -with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path=""$PWD""/testconfig --with-contenturl=https://cdn.download.clearlinux.org/update/ --with-versionurl=https://cdn.download.clearlinux.org/update/ --with-formatid=30 --with-build-number=123 --enable-debug"

	# Run tests without any security optimization and with address sanitizer
	CFLAGS="-fsanitize=address -fno-omit-frame-pointer -Werror" run_build --disable-optimizations "$configure_args"
	run_checks

	# Run tests with --force-tartar
	run_build --enable-force-tartar "$configure_args"
	run_checks_tartar

	# Run build with basic configurations
	CFLAGS="-O3 -falign-functions=32 -ffat-lto-objects -flto=4 -fno-math-errno -fno-semantic-interposition -fno-trapping-math -Werror" run_build "$configure_args"
	./swupd -v |grep "+SIGVERIFY"
	./swupd -v |grep "+THIRDPARTY"
	run_checks

	## Make sure build isn't broken for other settings
	run_build --disable-signature-verification --disable-third-party
	./swupd -v |grep "\\-SIGVERIFY"
	./swupd -v |grep "\\-THIRDPARTY"
}

main "$@"
