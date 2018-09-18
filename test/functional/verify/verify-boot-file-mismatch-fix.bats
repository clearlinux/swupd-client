#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	# change the content of testfile so the hash doesn't match
	write_to_protected_file -a "$TARGETDIR"/usr/lib/kernel/testfile "some new content"

}

@test "verify fix incorrect boot file" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Fixing modified files
		.Hash mismatch for file: .*/target-dir/usr/lib/kernel/testfile
		.fixed
		Inspected 7 files
		  0 files were missing
		  1 files did not match
		    1 of 1 files were fixed
		    0 of 1 files were not fixed
		  0 files found which should be deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
