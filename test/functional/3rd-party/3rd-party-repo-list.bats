#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

metadata_setup(){

	create_test_environment "$TEST_NAME"

	contents=$(cat <<- EOM
		[test1]
		url=https://www.abc.com

		[test2]
		url=https://www.efg.com

		[test4]
		url=https://www.pqr.com

		[test5]
		url=ERROR

		[test5]
		url=https://www.lmn.com
		\n
	EOM
	)

	repo_config_file="$PATH_PREFIX"/"$THIRD_PARTY_DIR"/repo.ini
	run sudo sh -c "mkdir -p $STATEDIR/3rd-party/"
	run sudo sh -c "mkdir -p $PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/{test1,test2,test3,test4,test5}"
	write_to_protected_file -a "$repo_config_file" "$contents"

}

@test "TPR007: Basic List of third-party entire repositories" {

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Swupd 3rd-party repositories:
		 - test1: https://www.abc.com
		 - test2: https://www.efg.com
		 - test4: https://www.pqr.com
		 - test5: https://www.lmn.com

		Total: 4
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=1
