#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "THP001: Basic test, No single repo or repo config dir" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

}

@test "THP002: Basic test, Listing a single repo" {

	create_repo_config
	create_repo test-repo1 www.xyz.com
	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repo
			--------
			name:	test-repo1
			url:	www.xyz.com
	EOM
	)
	assert_is_output "$expected_output"

}

