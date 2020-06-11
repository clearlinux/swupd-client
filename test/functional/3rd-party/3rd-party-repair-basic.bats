#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1

	# add a 3rd-party repo with some "findings" for diagnose
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1,/bar/file_2 -u test-repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 1 test-repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1 test-repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2 test-repo1
	update_bundle    "$TEST_NAME" test-bundle1 --add    /baz/file_3 test-repo1
	set_current_version "$TEST_NAME" 20 test-repo1

	# add another 3rd-party repo that has nothing to get fixed
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo2
	create_bundle -L -t -n test-bundle2 -f /baz/file_3 -u test-repo2 "$TEST_NAME"

	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bat
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo2/usr/untracked_file3

}

@test "TPR048: Repair a system that has a corrupt bundles from a 3rd-party repo" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2 -> deleted
		Inspected 19 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 15 files
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR049: Repair 3rd-party bundles from a specific repo" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --repo test-repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 15 files
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR050: Repair 3rd-party bundles to a specific version" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --version 10 --force --repo test-repo1"

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
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release -> fixed
		Removing extraneous files
		Inspected 17 files
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR051: Repair 3rd-party bundles using the picky option" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --picky"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr
		Inspected 19 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Removing extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/usr
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/usr/untracked_file3 -> deleted
		Inspected 16 files
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR052: Repair 3rd-party bundles using the picky option specifying the tree" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --picky --picky-tree /bat"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat/untracked_file1 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat/ -> deleted
		Inspected 21 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Removing extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/bat
		Inspected 15 files
		Calling post-update helper scripts
		Regenerating 3rd-party bundle binaries...
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=40
