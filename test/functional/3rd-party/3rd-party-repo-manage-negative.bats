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

@test "THP003: Negative test, repo list when no repo config exists" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

}

@test "THP004: Negative test, repo rmeove when no repo config exists" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

}

@test "THP005: Negative test, Remove repo which does not exist" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 www.xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	# remove random repo which does not exist
	run sudo sh -c "$SWUPD 3rd-party remove test-repo2 $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
			Repository not found
	EOM
	)
	assert_is_output "$expected_output"

}

@test "THP006: Negative test, repo add usage" {

	run sudo sh -c "$SWUPD 3rd-party add $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: The positional args: repo-name and URL are missing
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: The positional args: repo-URL is missing
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 www.abc.com junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: Unexpected arguments
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "THP007: Negative test, repo remove usage" {

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: The positional args: repo-URL is missing
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party remove test-repo1 junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: Unexpected arguments
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "THP008: Negative test, repo list usage" {

	run sudo sh -c "$SWUPD 3rd-party list junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Error: Unexpected arguments
			*.*
	EOM
	)
	assert_regex_in_output "$expected_output"

}
