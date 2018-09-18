#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -d /testdir "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /testdir .d..
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir "$zero_hash"

}

@test "verify delete already removed directory" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Adding any missing files
		Fixing modified files
		Deleted .*/target-dir/testdir
		Inspected 1 files
		  0 files were missing
		  1 files found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_not_exists "$TARGETDIR"/testdir

}
