#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

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

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 http://abc.com junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR004: Negative test, Add an already added repo" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 https://www.xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 https://abc.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_COULDNT_WRITE_FILE"
	expected_output=$(cat <<-EOM
		Error: The repo: test-repo1 already exists
		Error: Failed to add repo: test-repo1 to config
	EOM
	)
	assert_is_output "$expected_output"

}
