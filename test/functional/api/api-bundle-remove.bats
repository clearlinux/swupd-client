#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /file_2 "$TEST_NAME"

}

@test "API017: bundle-remove" {

	run sudo sh -c "$SWUPD bundle-remove test-bundle1 $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API080: bundle-remove --orphans" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

#WEIGHT=2
