#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 99990 10 staging

	# start slow response web server
	start_web_server -s

	# Set the web server as our upstream server
	port=$(get_web_server_port "$TEST_NAME")
	set_upstream_server "$TEST_NAME" "http://localhost:$port/$TEST_NAME/web-dir"

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"
}

@test "CHK003: Check for available updates with a slow server" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		There is a new OS version available: 99990
	EOM
	)
	assert_is_output "$expected_output"
}
