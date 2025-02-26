#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	create_bundle -L -n test-bundle2 -f /usr/foo/file_2,/usr/foo/file_3,/bar/file_4,/bar/file_5 "$TEST_NAME"
	set_current_version "$TEST_NAME" 20

}

@test "REP025: Repair using an older version won't remove an installed bundle that was not available then" {

	run sudo sh -c "$SWUPD repair --picky --version=10 $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: The --picky option is specified; ignoring version mismatch for repair
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Downloading missing manifests...
		Error: Unable to verify. One or more currently installed bundles are not available at version 10. Use --force to override
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/foo/file_2
	assert_file_exists "$TARGET_DIR"/usr/foo/file_3
	assert_file_exists "$TARGET_DIR"/bar/file_4
	assert_file_exists "$TARGET_DIR"/bar/file_5
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2

}

@test "REP026: Repair can be forced to remove installed bundles that were not available in a previous version" {

	run sudo sh -c "$SWUPD repair --picky --version=10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: The --force option is specified; ignoring version mismatch for repair
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		Removing extraneous files
		Removing extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/contenturl -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/clear/bundles/test-bundle2 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/file_3 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/file_2 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/ -> deleted
		Inspected 19 files
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		  6 files found which should be deleted
		    6 of 6 files were deleted
		    0 of 6 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/usr/foo/file_2
	assert_file_not_exists "$TARGET_DIR"/usr/foo/file_3
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2
	# these files should still exist in the target system since they were not in /usr
	assert_file_exists "$TARGET_DIR"/bar/file_4
	assert_file_exists "$TARGET_DIR"/bar/file_5

}

@test "REP027: Repair can remove files in a specified location that were not available in a previous version" {

	run sudo sh -c "$SWUPD repair --picky --picky-tree=/bar --version=10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: The --force option is specified; ignoring version mismatch for repair
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		Removing extraneous files
		Removing extra files under $ABS_TARGET_DIR/bar
		 -> Extra file: $ABS_TARGET_DIR/bar/file_5 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/bar/file_4 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/bar/ -> deleted
		Inspected 16 files
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/bar/file_4
	assert_file_not_exists "$TARGET_DIR"/bar/file_5
	# these files should still exist in the target system since we specified /bar
	# as the dir to look into
	assert_file_exists "$TARGET_DIR"/usr/foo/file_2
	assert_file_exists "$TARGET_DIR"/usr/foo/file_3
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2

}
#WEIGHT=12
