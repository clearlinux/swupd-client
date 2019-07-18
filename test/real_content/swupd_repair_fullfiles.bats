#!/usr/bin/env bats

load "real_content_lib"

# Test massive fullfile downloads
@test "RC002: Repair a big system" {
	# shellcheck disable=SC2153
	print "Install minimal system with newest version"
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -F $FORMAT"
	assert_status_is 0

	install_bundles -r
	verify_system
}
