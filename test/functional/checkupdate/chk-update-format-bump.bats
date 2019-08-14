#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2

}

@test "CHK010: Check for available updates accross format bumps" {

	# check-update should return the latest available version regardless
	# of if it is in a different format

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 40
		There is a new OS version available: 40
	EOM
	)
	assert_is_output "$expected_output"

}
