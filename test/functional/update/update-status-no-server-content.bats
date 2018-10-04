#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	sudo rm -rf "$WEBDIR"/version

}

@test "update --status with no server content" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"
	assert_status_is 2
	expected_output=$(cat <<-EOM
		Error: Unable to determine the server version
		Current OS version: 10
	EOM
	)
	assert_is_output "$expected_output"
}

