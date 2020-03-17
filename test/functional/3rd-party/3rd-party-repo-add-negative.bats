#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

metadata_setup() {

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

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$TP_BASE_DIR/test-repo1 junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR004: Negative test, Add an already added repo" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$TP_BASE_DIR/test-repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository added successfully
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 https://abc.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Adding 3rd-party repository test-repo1...
		Error: The repository test-repo1 already exists
		Failed to add repository
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
