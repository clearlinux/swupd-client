#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1

}

@test "API072: 3rd-party remove" {

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
