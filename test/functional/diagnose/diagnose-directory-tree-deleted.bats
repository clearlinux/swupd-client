#!/usr/bin/env bats

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


@test "DIA006: Diagnose a system that has a file that should be deleted" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		.* File that should be deleted: .*/target-dir/testdir1/testdir2/testfile
		.* File that should be deleted: .*/target-dir/testdir1/testdir2
		.* File that should be deleted: .*/target-dir/testdir1
		Inspected 4 files
		  3 files found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_exists "$TARGET_DIR"/testdir1
	assert_dir_exists "$TARGET_DIR"/testdir1/testdir2
	assert_file_exists "$TARGET_DIR"/testdir1/testdir2/testfile

}
#WEIGHT=2
