#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup(){

	create_test_environment "$TEST_NAME"

	# create a few 3rd-party repos within the test environment and add
	# some bundles to them
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -L -t -n test-bundle1 -f /file_1,/bin/binary_1 -u repo1 "$TEST_NAME"

}

@test "TPR009: Negative test, repo remove usage" {

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: The positional args: repo-name is missing
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party remove test-repo1 junk_positional $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR010: Negative test, Remove a repo which does not exist" {

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS bad_name "
	assert_status_is "$SWUPD_INVALID_REPOSITORY"
	expected_output=$(cat <<-EOM
		Error: 3rd-party repository bad_name was not found
		Failed to remove repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR011: Negative test, Remove a repo on a new system" {

	run sudo sh -c "rm -r $STATEDIR/3rd-party"

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS repo1"
	assert_status_is "$SWUPD_INVALID_REPOSITORY"
	expected_output=$(cat <<-EOM
		Error: 3rd-party repository repo1 was not found
		Failed to remove repository
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=11
