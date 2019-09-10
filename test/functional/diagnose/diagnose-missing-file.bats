#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/test-file1,/bar/test-file2 "$TEST_NAME"
	# remove a directory and file that are part of the bundle
	sudo rm -rf "$TARGETDIR"/foo/test-file1

}

@test "DIA002: Diagnose a system that is missing a file" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Download missing manifests
		Checking for missing files
		.* Missing file: .*/target-dir/foo/test-file1
		Checking for corrupt files
		Checking for extraneous files
		Inspected 7 files
		  1 file was missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}
