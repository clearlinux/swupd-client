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

mkdir -p /build
cd /build

if [[ ! -d /source ]]; then
	echo "Missing source tree!" >&2
	exit 1
fi

# Copy all the files across to prevent contaminating the bind mount
cp -Ra /source/. /build

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

jobCount="-j$(($(getconf _NPROCESSORS_ONLN)+1))"

# Fix file permissions in the event of dodgy umask
find test/functional -exec chmod g-w {} \;

# Configure the build
autoreconf --verbose --warnings=none --install --force || die_fatal "Configure failure"
./configure || die_fatal "Configure failure"

# Build error
make $jobCount || die_fatal "Build failure"

# Test suite failure, print the log
make $jobCount check || die_log_fatal
