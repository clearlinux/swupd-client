#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 99990 10 staging

	# start slow response web server
	start_web_server -s -l 1

	# Set the web server as our upstream server
	port=$(get_web_server_port "$TEST_NAME")
	set_upstream_server "$TEST_NAME" "http://localhost:$port/$TEST_NAME/web-dir"

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"
}

@test "CHK003: Check for available updates with a slow server" {

	run sudo sh -c "$SWUPD check-update --allow-insecure-http $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Current OS version: 10
		Latest server version: 99990
		There is a new OS version available: 99990
	EOM
	)
	assert_is_output "$expected_output"
}
#WEIGHT=4
