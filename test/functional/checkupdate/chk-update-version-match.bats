#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"

}

@test "CHK002: Check for available updates when we are at latest" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is 1
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=1
