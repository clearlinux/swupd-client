#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -d /foo "$TEST_NAME"
	# remove the directory /usr/bin from the target-dir
	sudo rm -rf "$TARGETDIR"/foo

}

@test "verify check missing directory" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		.Hash mismatch for file: .*/target-dir/foo
		Inspected 4 files
		  1 files did not match
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_not_exists "$TARGETDIR"/foo

}

