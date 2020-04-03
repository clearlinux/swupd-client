#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_path=$(realpath "$TEST_NAME")
repo="$test_path"/3rd-party/test-repo1

global_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo "$TEST_NAME" 10 staging test-repo1

}

@test "TPR003: Negative test, repo add usage" {

	run sudo sh -c "$SWUPD 3rd-party add $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: The positional args: repo-name and repo-url are missing
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: The positional args: repo-url is missing
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$repo junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR004: Negative test, Add an already added repo" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$repo $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository added successfully
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$repo $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: A 3rd-party repository called "test-repo1" already exists
		Failed to add repository
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
