#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 60 1 repo1
	create_third_party_repo -a "$TEST_NAME" 20 5 repo2

}

@test "API002: 3rd-party info" {

	run sudo sh -c "$SWUPD 3rd-party info $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		60
		[repo2]
		20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API003: 3rd-party info with --repo" {

	run sudo sh -c "$SWUPD 3rd-party info $SWUPD_OPTS --quiet --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		60
	EOM
	)
	assert_is_output "$expected_output"

}
