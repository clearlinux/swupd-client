#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment -e "$TEST_NAME"
	set_latest_version "$TEST_NAME" 100

}

test_setup() {

	:

}

test_teardown() {

	:

}

global_teardown() { 

	destroy_test_environment "$TEST_NAME"

}

@test "UPD034: Show current OS version and latest version available on server" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 100
		There is a new OS version available: 100
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD035: Show current OS version when os-release has double quote on version" {

	set_current_version "$TEST_NAME" "\"10\""

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 100
		There is a new OS version available: 100
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD036: Show current OS version when os-release has single quote on version" {

	set_current_version "$TEST_NAME" "\\'10\\'"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 100
		There is a new OS version available: 100
	EOM
	)
	assert_is_output "$expected_output"

}
