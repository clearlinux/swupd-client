#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/test-file1,/bar/test-file2 "$TEST_NAME"
	# remove a directory and file that are part of the bundle
	sudo rm -rf "$TARGETDIR"/foo/test-file1

}

@test "VER002: Verify a system that is missing a file" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Missing file: .*/target-dir/foo/test-file1
		Inspected 7 files
		  1 file was missing
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}

@test "VER003: Verify fixes a system that is missing a file" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/foo/test-file1
		.fixed
		Fixing modified files
		Inspected 7 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}
