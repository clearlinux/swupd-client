#!/usr/bin/env bats

load "real_content_lib"

# Test massive fullfile downloads
@test "RCT002: Repair a big system" {
	print "Install minimal system with newest version"
	# shellcheck disable=SC2153
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -F $FORMAT"
	assert_status_is 0

	install_bundles -r
	verify_system
}
