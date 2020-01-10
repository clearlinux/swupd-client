#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup(){

	create_test_environment "$TEST_NAME"

	contents=$(cat <<- EOM
		\n
		[test1]
		url=www.abc.com

		[test2]
		url=www.efg.com

		[test3]
		url=www.xyz.com

		[test4]
		url=www.pqr.com
		invalid=456

		[test5]
		url=www.lmn.com
		\n
	EOM
	)

	repo_config_file="$STATEDIR"/3rd-party/repo.ini
	sudo mkdir -p "$STATEDIR"/3rd-party
	sudo mkdir -p "$PATH_PREFIX"/opt/3rd-party/{test1,test2,test3,test4,test5}
	write_to_protected_file -a "$repo_config_file" "$contents"
	sudo mkdir "$STATEDIR"/3rd-party/{test1,test2,test3,test4,test5}

}

test_teardown(){

	destroy_test_environment "$TEST_NAME"

}

@test "TPR008: Remove multiple repos" {

	repo_config_file="$STATEDIR"/3rd-party/repo.ini

	#remove at start of file
	assert_dir_exists "$PATH_PREFIX"/opt/3rd-party/test1
	assert_dir_exists "$STATEDIR"/3rd-party/test1
	run sudo sh -c "$SWUPD 3rd-party remove test1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Removing repository test1...
		Repository and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX"/opt/3rd-party/test1
	assert_dir_not_exists "$STATEDIR"/3rd-party/test1

	#remove at middle of file
	assert_dir_exists "$PATH_PREFIX"/opt/3rd-party/test3
	assert_dir_exists "$STATEDIR"/3rd-party/test3
	run sudo sh -c "$SWUPD 3rd-party remove test3 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Removing repository test3...
		Repository and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX"/opt/3rd-party/test3
	assert_dir_not_exists "$STATEDIR"/3rd-party/test3

	#remove at end of file
	assert_dir_exists "$PATH_PREFIX"/opt/3rd-party/test5
	assert_dir_exists "$STATEDIR"/3rd-party/test5
	run sudo sh -c "$SWUPD 3rd-party remove test5 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Removing repository test5...
		Repository and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX"/opt/3rd-party/test5
	assert_dir_not_exists "$STATEDIR"/3rd-party/test5

	expected_contents=$(cat <<- EOM
		[test2]
		url=www.efg.com

		[test4]
		url=www.pqr.com
	EOM
	)

	run sudo sh -c "cat $repo_config_file"
	assert_is_output --identical "$expected_contents"

}
