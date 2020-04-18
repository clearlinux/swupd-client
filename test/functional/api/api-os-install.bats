#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

}

@test "API062: os-install" {

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
#WEIGHT=1
