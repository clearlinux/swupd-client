#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2

}

@test "VER020: Verify enforces the use of the correct format" {

	run sudo sh -c "$SWUPD verify --fix --format=1 --manifest=40 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$SWUPD_COULDNT_LOAD_MANIFEST"
	expected_output=$(cat <<-EOM
		Verifying version 40
		Error: Mismatching formats detected when verifying 40 (expected: 1; actual: 2)
		Latest supported version to verify: 20
		Fix did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
