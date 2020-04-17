#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo "$TEST_NAME" 10 staging repo1
	export repo1="$TPURL"

}

@test "API071: 3rd-party add --certpath CERTIFICATE" {

	run sudo sh -c "$SWUPD 3rd-party add $SWUPD_OPTS my-repo file://$repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
