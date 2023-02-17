#!/bin/bash
#
# This file is part of swupd-client.
#
# Copyright © 2017 Intel Corporation
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2 or later of the License.
#

# Enforce correct umask
umask 0022

die_fatal()
{
	echo "$*"
	exit 1
}

die_log_fatal()
{
	if [[ -e test-suite.log ]]; then
		cat test-suite.log
	else
		echo "test-suite.log is missing." >&2
	fi
	exit 1
}
sudo chown -R tester .

# Fix file permissions in the event of dodgy umask
find test/functional -exec chmod g-w {} \;

# Configure the build
autoreconf --verbose --warnings=none --install --force || die_fatal "Configure failure"

./configure CFLAGS="$CFLAGS -fsanitize=address -Werror" --prefix=/usr --with-fallback-capaths="$PWD"/swupd_test_certificates --with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path=./testconfig --enable-debug || die_fatal "Configure failure"

make -j$(nproc) || die_fatal "Build Failure"
make compliant || die_fatal "make compliant failure"
make shellcheck-all || die_fatal "make shellcheck-all failure"
make docs-coverage || die_fatal "make docs-coverage failure"
sudo make install || die_fatal "make install failure"
make install-check || die_fatal "make install-check failure"
make check -j$(nproc) || die_fatal "unit test failure"
make distcheck -j$(nproc) || die_fatal "make distcheck failure"
./test/code_analysis/check_api_changes.bats || die_fatal "api change check failure"
./scripts/github_actions/run_check.bash 1 1 || die_log_fatal
FILES="$(find test/functional/only_in_ci_slow/ -name "*.bats")"
env RUNNING_IN_CI=true TESTS="$(echo $FILES)" make -e check || die_log_fatal
FILES="$(find test/functional/only_in_ci_system/ -name "*.bats")"
env SKIP_DATE_CHECK=true SKIP_DISK_SPACE=true RUNNING_IN_CI=true TESTS="$(echo $FILES)" make -e check || die_log_fatal
