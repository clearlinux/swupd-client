#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/test-file1,/bar/test-file2 "$TEST_NAME"
	# remove a directory and file that are part of the bundle
	sudo rm -rf "$TARGETDIR"/foo/test-file1

}

@test "REP021: Repair a system that is missing a file" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		.* Missing file: .*/target-dir/foo/test-file1 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 7 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}
