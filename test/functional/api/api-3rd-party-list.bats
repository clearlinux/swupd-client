#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	export repo1="$ABS_TP_URL"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	export repo2="$ABS_TP_URL"

}

@test "API073: 3rd-party list" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		repo1: file://$repo1
		repo2: file://$repo2
	EOM
	)
	assert_is_output "$expected_output"

}
