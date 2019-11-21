#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup(){

	create_test_environment "$TEST_NAME"

	contents=$(cat <<- EOM
		[test1]
		url=https://www.abc.com
		version=0

		[test2]
		url=https://www.efg.com
		version=0

		[test4]
		url=https://www.pqr.com
		version=0

		[test5]
		url=ERROR
		version=0

		[test5]
		url=https://www.lmn.com
		version=0
		\n
	EOM
	)

	repo_config_file="$STATEDIR"/3rd_party/repo.ini
	run sudo sh -c "mkdir -p $STATEDIR/3rd_party/"
	run sudo sh -c "mkdir -p $PATH_PREFIX/opt/3rd-party/{test1,test2,test3,test4,test5}"
	write_to_protected_file -a "$repo_config_file" "$contents"

}

test_teardown(){

	destroy_test_environment "$TEST_NAME"

}

@test "TPR007: Basic List of third-party entire repositories" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Swupd 3rd party repositories found: 4
		test1: https://www.abc.com
		test2: https://www.efg.com
		test4: https://www.pqr.com
		test5: https://www.lmn.com
	EOM
	)
	assert_is_output --identical "$expected_output"

}
