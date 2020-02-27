#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

}

@test "REP019: Repair does not delete ghosted files from the system" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 2 files
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, despite being marked as "ghosted" in the
	# Manifest and treated as deleted for parts of swupd-client.
	assert_file_exists "$TARGETDIR"/foo

}
#WEIGHT=2
