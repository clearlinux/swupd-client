#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 20

}

@test "API007: checkupdate (update available)" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API008: checkupdate (up to date)" {

	# set current version to 20 so there are no updates available
	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_NO"
	assert_output_is_empty

}
#WEIGHT=3
