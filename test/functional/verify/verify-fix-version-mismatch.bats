#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --add-dir /usr/bin
	set_current_version "$TEST_NAME" 20

}

@test "verify version mismatch enforcement" {

	run sudo sh -c "$SWUPD verify --fix -m 10 $SWUPD_OPTS"
	assert_status_is "$EMANIFEST_LOAD"
	expected_output=$(cat <<-EOM
		Verifying version 10
		ERROR: Fixing to a different version requires --force or --picky
		Error: Fix did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}