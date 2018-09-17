#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

	# start slow response web server
	start_web_server -s
}

test_setup() {

	return
}

test_teardown() {

	return
}

global_teardown() {

	destroy_web_server
	destroy_test_environment "$TEST_NAME"
}

@test "check-update with a slow server" {

	# Pre-req: create a web server that can serve as a slow content download server
	slow_opts="-p $TEST_NAME/target-dir -F staging -u http://localhost:$PORT/"

	# test
	run sudo sh -c "$SWUPD check-update $slow_opts"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		There is a new OS version available: 99990
	EOM
	)
	assert_is_output "$expected_output"
}
