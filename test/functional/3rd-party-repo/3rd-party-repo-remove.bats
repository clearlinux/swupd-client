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
		version=0

		[test2]
		url=www.efg.com
		version=0

		[test3]
		url=www.xyz.com
		version=0

		[test4]
		url=www.pqr.com
		version=123
		invalid=456

		[test5]
		url=www.lmn.com
		version=0
		\n
	EOM
	)

	repo_config_file="$STATEDIR"/3rd_party/repo.ini
	run sudo sh -c "mkdir -p $STATEDIR/3rd_party/"
	run sudo sh -c "mkdir -p $PATH_PREFIX/opt/3rd_party/{test1,test2,test3,test4,test5}"
	write_to_protected_file -a "$repo_config_file" "$contents"

}

test_teardown(){

	destroy_test_environment "$TEST_NAME"

}

@test "TPR008: Remove multiple repos" {

	repo_config_file="$STATEDIR"/3rd_party/repo.ini

	#remove at start of file
	run sudo sh -c "$SWUPD 3rd-party remove test1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository test1 and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test1"

	#remove at middle of file
	run sudo sh -c "$SWUPD 3rd-party remove test3 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository test3 and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test1"

	#remove at end of file
	run sudo sh -c "$SWUPD 3rd-party remove test5 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repository test5 and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test1"

	expected_contents=$(cat <<- EOM
		[test2]
		url=www.efg.com
		version=0

		[test4]
		url=www.pqr.com
		version=123
	EOM
	)

	run sudo sh -c "cat $repo_config_file"
	assert_is_output --identical "$expected_contents"

}
