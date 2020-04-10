#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -n test-bundle1 -f /file_1 -u repo1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2a -u repo1 "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -n test-bundle2 -f /file_2b -u repo2 "$TEST_NAME"

}

@test "API021: 3rd-party bundle-add" {

	run sudo sh -c "$SWUPD 3rd-party bundle-add test-bundle1 $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API022: 3rd-party bundle-add with --repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-add test-bundle1 $SWUPD_OPTS --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
