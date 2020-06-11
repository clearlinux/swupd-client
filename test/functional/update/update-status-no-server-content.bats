#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	sudo rm -rf "$WEB_DIR"/version

}

@test "UPD037: Try showing latest version available on server when there is no server content" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"
	expected_output=$(cat <<-EOM
		Error: Unable to determine the server version
		Current OS version: 10
	EOM
	)
	assert_in_output "$expected_output"
}
#WEIGHT=1
