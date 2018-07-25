#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove the version from web-dir so there is no "server" data
	sudo rm -rf "$TEST_NAME"/web-dir/version

}

@test "check-update with no server version file available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS_NO_CERT"
	assert_status_is_not 0
	assert_is_output "Error: server does not report any version"

}
