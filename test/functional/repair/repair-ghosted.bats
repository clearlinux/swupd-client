#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /foo "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /foo .g..

}

@test "REP019: Repair does not delete ghosted files from the system" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Adding any missing files
		Repairing modified files
		Inspected 2 files
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, despite being marked as "ghosted" in the
	# Manifest and treated as deleted for parts of swupd-client.
	assert_file_exists "$TARGETDIR"/foo

}
