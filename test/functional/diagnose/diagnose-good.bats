#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"

}

@test "DIA001: Diagnose installed content on a system that is fine" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Inspected 11 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
