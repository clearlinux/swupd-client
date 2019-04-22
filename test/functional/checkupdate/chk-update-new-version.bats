#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# create a new version 100
	create_test_environment "$TEST_NAME" 100
	# this will leave both latest version and current version as 100
	# so change current version back to 10
	set_current_version "$TEST_NAME" 10

}

@test "CHK001: Check for available updates when there is a new version available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 100
		There is a new OS version available: 100
	EOM
	)
	assert_is_output "$expected_output"

}
