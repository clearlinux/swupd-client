#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove the version from web-dir so there is no "server" data
	sudo rm -rf "$TEST_NAME"/web-dir/version

}

@test "CHK004: Check for available updates with no server version file available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Error: Unable to determine the server version
		Current OS version: 10
	EOM
	)
	assert_in_output "$expected_output"

}
