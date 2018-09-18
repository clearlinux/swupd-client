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
		Current OS version: 10
		Cannot get the latest server version. Could not reach server
	EOM
	)
	assert_is_output "$expected_output"
}

