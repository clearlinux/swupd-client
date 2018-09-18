#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /testdir1/testdir2/testfile "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /testdir1/testdir2/testfile .d..
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /testdir1/testdir2 .d..
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /testdir1 .d..
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir1/testdir2/testfile "$zero_hash"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir1/testdir2 "$zero_hash"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir1 "$zero_hash"

}


@test "verify delete directory tree" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Adding any missing files
		Fixing modified files
		Deleted .*/target-dir/testdir1/testdir2/testfile
		Deleted .*/target-dir/testdir1/testdir2
		Deleted .*/target-dir/testdir1
		Inspected 1 files
		  0 files were missing
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_not_exists "$TARGETDIR"/testdir1
	assert_dir_not_exists "$TARGETDIR"/testdir1/testdir2
	assert_file_not_exists "$TARGETDIR"/testdir1/testdir2/testfile

}
