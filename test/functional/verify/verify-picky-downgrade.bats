#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	create_bundle -L -n test-bundle2 -f /usr/foo/file_2,/usr/foo/file_3,/bar/file_4,/bar/file_5 "$TEST_NAME"
	set_current_version "$TEST_NAME" 20

}

@test "VER027: Verify using an older version won't remove an installed bundle that was not available then" {

	run sudo sh -c "$SWUPD verify --fix --picky --manifest=10 $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Warning: the force or picky option is specified; ignoring version mismatch for verify --fix
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Error: Unable to verify, one or more currently installed bundles are not available at version 10. Use --force to override.
		Fix did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/foo/file_2
	assert_file_exists "$TARGETDIR"/usr/foo/file_3
	assert_file_exists "$TARGETDIR"/bar/file_4
	assert_file_exists "$TARGETDIR"/bar/file_5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2

}

@test "VER028: Verify can be forced to remove installed bundles that were not available in a previous version" {

	run sudo sh -c "$SWUPD verify --fix --picky --manifest=10 --force $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Warning: the force or picky option is specified; ignoring version mismatch for verify --fix
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Warning: One or more installed bundles that are not available at version 10 will be removed.
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Fixing modified files
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		--picky removing extra files under .*/target-dir/usr
		REMOVING /usr/share/defaults/swupd/versionurl
		REMOVING /usr/share/defaults/swupd/contenturl
		REMOVING /usr/share/clear/bundles/test-bundle2
		REMOVING /usr/foo/file_3
		REMOVING /usr/foo/file_2
		REMOVING DIR /usr/foo/
		Inspected 19 files
		  1 file did not match
		    1 of 1 files were fixed
		    0 of 1 files were not fixed
		  6 files found which should be deleted
		    6 of 6 files were deleted
		    0 of 6 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/usr/foo/file_2
	assert_file_not_exists "$TARGETDIR"/usr/foo/file_3
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	# these files should still exist in the target system since they were not in /usr
	assert_file_exists "$TARGETDIR"/bar/file_4
	assert_file_exists "$TARGETDIR"/bar/file_5

}

@test "VER029: Verify can remove files in a specified location that were not available in a previous version" {

	run sudo sh -c "$SWUPD verify --fix --picky --picky-tree=/bar --manifest=10 --force $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Warning: the force or picky option is specified; ignoring version mismatch for verify --fix
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Warning: One or more installed bundles that are not available at version 10 will be removed.
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Fixing modified files
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		--picky removing extra files under .*/target-dir/bar
		REMOVING /bar/file_5
		REMOVING /bar/file_4
		REMOVING DIR /bar/
		Inspected 16 files
		  1 file did not match
		    1 of 1 files were fixed
		    0 of 1 files were not fixed
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/bar/file_4
	assert_file_not_exists "$TARGETDIR"/bar/file_5
	# these files should still exist in the target system since we specified /bar
	# as the dir to look into
	assert_file_exists "$TARGETDIR"/usr/foo/file_2
	assert_file_exists "$TARGETDIR"/usr/foo/file_3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2

}

@test "VER030: Verify can show files that would be removed if not available in a previous version" {

	run sudo sh -c "$SWUPD verify --picky --manifest=10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Warning: One or more installed bundles are not available at version 10.
		Generating list of extra files under .*/target-dir/usr
		/usr/share/defaults/swupd/versionurl
		/usr/share/defaults/swupd/contenturl
		/usr/share/clear/bundles/test-bundle2
		/usr/foo/file_3
		/usr/foo/file_2
		/usr/foo/
		Inspected 19 files
		  6 files found which should be deleted
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/foo/file_2
	assert_file_exists "$TARGETDIR"/usr/foo/file_3
	assert_file_exists "$TARGETDIR"/bar/file_4
	assert_file_exists "$TARGETDIR"/bar/file_5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/defaults/swupd/versionurl
	assert_file_exists "$TARGETDIR"/usr/share/defaults/swupd/contenturl

}
