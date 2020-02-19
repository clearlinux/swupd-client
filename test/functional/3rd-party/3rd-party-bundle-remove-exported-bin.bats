#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# create a 3rd-party bundle that has a couple of binaries
	add_third_party_repo "$TEST_NAME" 10 1 test-repo1
	create_bundle -L -n test-bundle1 -f /file1,/foo/file_2,/usr/bin/file_3,/bin/file_4 -u test-repo1 "$TEST_NAME"
	# create a 3rd-party bundle that shares the same binary
	create_bundle -L -n test-bundle2 -f /file2,/usr/bin/file_3                         -u test-repo1 "$TEST_NAME"
}

@test "TPR058: Removing one bundle from a third party repo that exported binaries" {

	# when deleting a 3rd-party bundle, all the binaries exported by the
	# bundle should be removed as well

	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/file_3
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/file_4

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo1
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

	# non-confilcting scripts to binaries should have been removed
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/file_4

	# script to binaries that are still being used by another bundle should not be removed
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/file_3

}
#WEIGHT=5
