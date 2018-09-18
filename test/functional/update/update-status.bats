#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	set_latest_version "$TEST_NAME" 100

}

@test "update --status" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 100
	EOM
	)
	assert_is_output "$expected_output"

}
