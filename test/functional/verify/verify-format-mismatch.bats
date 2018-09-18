#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -u "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -u "$TEST_NAME" 40 30 2

}

@test "verify format mismatch enforcement" {

	run sudo sh -c "$SWUPD verify --fix -F 1 -m 40 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$EMANIFEST_LOAD"
	expected_output=$(cat <<-EOM
		Verifying version 40
		ERROR: Mismatching formats detected when verifying 40 (expected: 1; actual: 2)
		Latest supported version to verify: 20
		Error: Fix did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
