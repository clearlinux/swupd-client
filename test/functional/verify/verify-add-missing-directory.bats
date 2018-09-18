#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -d /foo/bar "$TEST_NAME"
	# remove the directory that is part of the bundle
	sudo rm -rf "$TARGETDIR"/foo/bar

}

@test "verify add missing directory" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Missing file: .*/target-dir/foo/bar
		.fixed
		Fixing modified files
		Inspected 5 files
		  1 files were missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		  0 files found which should be deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_exists "$TEST_NAME"/target-dir/foo/bar

}
