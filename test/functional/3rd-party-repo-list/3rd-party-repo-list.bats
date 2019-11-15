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

	repo_config_file="$STATEDIR"/3rd_party/repo.ini
	run sudo sh -c "mkdir -p $STATEDIR/3rd_party/"
	run sudo sh -c "mkdir -p $PATH_PREFIX/opt/3rd-party/{test1,test2,test3,test4,test5}"
	write_to_protected_file -a "$repo_config_file" "$contents"

}

test_teardown(){

	run sudo sh -c "rm -r $PATH_PREFIX/opt/3rd-party"
	run sudo sh -c "rm -r $STATEDIR/3rd_party"
	destroy_test_environment "$TEST_NAME"

}

@test "TRA006: Basic List of third-party entire repositories" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Repo
		--------
		name:	test1
		url:	www.abc.com

		Repo
		--------
		name:	test2
		url:	www.efg.com

		Repo
		--------
		name:	test4
		url:	www.pqr.com

		Repo
		--------
		name:	test5
		url:	www.lmn.com
	EOM
	)
	assert_is_output --identical "$expected_output"

}
