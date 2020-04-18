#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle1 -f /file_1 -u repo1 "$TEST_NAME"

}

@test "API018: 3rd-party bundle-remove" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove test-bundle1 $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API019: 3rd-party bundle-remove with --repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove test-bundle1 $SWUPD_OPTS --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
#WEIGHT=10
