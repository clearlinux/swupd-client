#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -d /testdir "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /testdir .d..
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir "$zero_hash"

}

@test "REP010: Repair is able to remove an empty directory" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		.* File that should be deleted: .*/target-dir/testdir -> deleted
		Inspected 2 files
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_not_exists "$TARGETDIR"/testdir

}
