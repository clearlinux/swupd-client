#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup(){

	create_test_environment "$TEST_NAME"

	contents=$(cat <<- EOM
		[test1]
		url=www.abc.com
		\n
		[test2]
		url=www.efg.com
		\n
		[test4]
		url=www.pqr.com
		\n
		[test5]
		url=www.lmn.com
		\n
	EOM
	)

	repo_config_file="$STATEDIR"/3rd-party/repo.ini
	write_to_protected_file -a "$repo_config_file" "$contents"

}

test_teardown(){

	destroy_test_environment "$TEST_NAME"

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

	run sudo sh -c "$SWUPD 3rd-party remove test3 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Removing repository test3...
			Error: Repository not found
			Failed to remove repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR011: Negative test, Remove a repo on a new system" {

	run sudo sh -c "rm -r $PATH_PREFIX/opt/3rd-party"
	run sudo sh -c "rm -r $STATEDIR/3rd-party"

	run sudo sh -c "$SWUPD 3rd-party remove test3 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
			Removing repository test3...
			Error: Repository not found
			Failed to remove repository
	EOM
	)
	assert_is_output "$expected_output"

}
