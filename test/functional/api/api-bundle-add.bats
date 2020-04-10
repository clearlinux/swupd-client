#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/file_1 "$TEST_NAME"

}

@test "API020: bundle-add" {

	run sudo sh -c "$SWUPD bundle-add test-bundle1 $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
