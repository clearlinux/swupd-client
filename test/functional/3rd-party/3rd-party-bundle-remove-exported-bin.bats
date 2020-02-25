#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	add_third_party_repo "$TEST_NAME" 10 1 repo1
	# create 3rd-party bundles that share a binary
	bin_file1=$(create_file -x "$TPWEBDIR"/10/files)
	create_bundle -L -n test-bundle1 -f /file1,/foo/file2,/usr/bin/bin_file1:"$bin_file1",/bin/bin_file2 -u repo1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /file3,/usr/bin/bin_file1:"$bin_file1"                           -u repo1 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /file1,/usr/bin/bin_file1:"$bin_file1"                           -u repo1 "$TEST_NAME"
	# let's make test-bundle3 contain /usr/bin/bin_file1 but not export it
	update_manifest "$TPWEBDIR"/10/Manifest.test-bundle3 file-status /usr/bin/bin_file1 F...

}

@test "TPR058: Removing one bundle from a third party repo that exported binaries" {

	# when deleting a 3rd-party bundle, all the binaries exported by the
	# bundle should be removed as well
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 5
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

	# non-confilcting binary scripts should have been removed
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2

	# script that are still being used by other bundles should not be removed
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1

}
#WEIGHT=5

@test "TPR070: Removing all third party bundles that exported a binary that is still needed but non-exported" {

	# when deleting a 3rd-party bundle that has an exported binary, if another bundle still needs
	# the binary but it did not export that binary, then the file should not be removed
	# from the system but the script that exports the binary should

	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 5
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository repo1
		The following bundles are being removed:
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 2
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

	# non-confilcting scripts to binaries should have been removed
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_not_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2
	assert_file_exists "$TPTARGETDIR"/usr/bin/bin_file1

}

@test "TPR071: Removing one third party bundle that has a non-exported binary but exported by another bundle" {

	# when deleting a 3rd-party bundle that has an non-exported binary,
	# if another bundle needsthe binary, then the file should not be removed

	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle3 in the 3rd-party repositories...
		Bundle test-bundle3 found in 3rd-party repository repo1
		The following bundles are being removed:
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 1
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_exists "$PATH_PREFIX"/"$THIRD_PARTY_BIN_DIR"/bin_file2
	assert_file_exists "$TPTARGETDIR"/usr/bin/bin_file1

}
