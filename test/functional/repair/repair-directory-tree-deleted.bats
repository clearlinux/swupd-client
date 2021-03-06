#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /testdir1/testdir2/testfile "$TEST_NAME"
	update_manifest -p "$WEB_DIR"/10/Manifest.os-core file-status /testdir1/testdir2/testfile .d..
	update_manifest -p "$WEB_DIR"/10/Manifest.os-core file-status /testdir1/testdir2 .d..
	update_manifest -p "$WEB_DIR"/10/Manifest.os-core file-status /testdir1 .d..
	update_manifest -p "$WEB_DIR"/10/Manifest.os-core file-hash /testdir1/testdir2/testfile "$ZERO_HASH"
	update_manifest -p "$WEB_DIR"/10/Manifest.os-core file-hash /testdir1/testdir2 "$ZERO_HASH"
	update_manifest "$WEB_DIR"/10/Manifest.os-core file-hash /testdir1 "$ZERO_HASH"

}

@test "REP009: Repair a system that has a file that should be deleted" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		.* File that should be deleted: .*/target-dir/testdir1/testdir2/testfile -> deleted
		.* File that should be deleted: .*/target-dir/testdir1/testdir2 -> deleted
		.* File that should be deleted: .*/target-dir/testdir1 -> deleted
		Inspected 4 files
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_not_exists "$TARGET_DIR"/testdir1
	assert_dir_not_exists "$TARGET_DIR"/testdir1/testdir2
	assert_file_not_exists "$TARGET_DIR"/testdir1/testdir2/testfile

}
#WEIGHT=12
