#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment -r "$TEST_NAME"

}

@test "DIA001: Diagnose installed content on a system that is fine" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Inspected 11 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=2
