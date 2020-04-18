#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1,/usr/bin/binary_1,/bin/binary_2,/bin/binary_3 -u repo1 "$TEST_NAME"

	# make a change to a file so we have somthing to repair
	write_to_protected_file -a "$TPTARGETDIR"/foo/file_1 "corrupting the file"
	write_to_protected_file -a "$TPTARGETDIR"/bin/binary_2 "corrupting the file"

	# add a line to the wrapper scripts for all binaries so we can tell
	# if they were re-generated after the repair
	write_to_protected_file -a "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1 "TEST_STRING"
	write_to_protected_file -a "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2 "TEST_STRING"
	write_to_protected_file -a "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3 "TEST_STRING"

}

@test "TPR081: Repair a system that has a bundle with corrupt wrapper scripts from a 3rd-party repo" {

	# pre-test checks
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3
	sudo grep -q "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1 || exit 1
	sudo grep -q "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2 || exit 1
	sudo grep -q "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3 || exit 1

	# every time a user repairs a system all the wrapper scripts for
	# binaries should be regenerated, not only those that were repaired

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________
		 3rd-Party Repo: repo1
		_______________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/repo1/bin/binary_2 -> fixed
		 -> Hash mismatch for file: $PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/repo1/foo/file_1 -> fixed
		Removing extraneous files
		Inspected 20 files
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3
	sudo grep -vq "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1 || exit 1
	sudo grep -vq "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2 || exit 1
	sudo grep -vq "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3 || exit 1

}
#WEIGHT=5
