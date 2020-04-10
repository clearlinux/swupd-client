#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --update /file_1

}

@test "API012: Update" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API013: Update (no update available)" {

	# set current version to 20 so there are no updates available
	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
