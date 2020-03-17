#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

metadata_setup(){

	create_test_environment "$TEST_NAME"
	# create a few 3rd-party repos within the test environment and add
	# some bundles to them
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -L -t -n test-bundle1 -f /file_1,/bin/binary_1           -u repo1 "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle2 -f /file_2,/bin/binary_2           -u repo2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle3 -f /file_3,/usr/bin/binary_3       -u repo2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle4 -f /file_4,/usr/local/bin/binary_4 -u repo2 "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 repo3
	create_bundle -L -t -n test-bundle5 -f /file_5,/usr/bin/binary_5       -u repo3 "$TEST_NAME"

}

@test "TPR008: Remove multiple repos" {

	#remove at start of file
	assert_dir_exists "$PATH_PREFIX"/"$THIRD_PARTY_BUNDLES_DIR"/repo1
	assert_dir_exists "$STATEDIR"/3rd-party/repo1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_3
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_4
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_5

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS repo2"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Removing repository repo2...
		Removing 3rd-party bundle binaries...
		Repository and its content removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_exists "$PATH_PREFIX"/"$THIRD_PARTY_BUNDLES_DIR"/repo1
	assert_dir_exists "$STATEDIR"/3rd-party/repo1
	assert_dir_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BUNDLES_DIR"/repo2
	assert_dir_not_exists "$STATEDIR"/3rd-party/repo2
	assert_dir_exists "$PATH_PREFIX"/"$THIRD_PARTY_BUNDLES_DIR"/repo3
	assert_dir_exists "$STATEDIR"/3rd-party/repo3
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_3
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_4
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/binary_5

}
#WEIGHT=11
