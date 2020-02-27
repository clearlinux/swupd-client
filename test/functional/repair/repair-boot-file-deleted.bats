#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

}

@test "REP004: Repair does not delete boot files from the system" {

	# <If necessary add a detailed explanation of the test here>

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 5 files
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
#WEIGHT=2
