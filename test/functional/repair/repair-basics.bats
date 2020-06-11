#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3
	set_current_version "$TEST_NAME" 20
	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGET_DIR"/bat
	sudo touch "$TARGET_DIR"/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGET_DIR"/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/usr/untracked_file3

}

@test "REP011: Repair modified files, new files and deleted files" {

	# repair should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# repair should not delete any untracked files

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Inspected 18 files
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
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGET_DIR"/foo/file_1
	assert_file_not_exists "$TARGET_DIR"/bar/file_2
	assert_file_exists "$TARGET_DIR"/baz/file_3
	# untracked files
	assert_file_exists "$TARGET_DIR"/bat/untracked_file1
	assert_file_exists "$TARGET_DIR"/bar/untracked_file2
	assert_file_exists "$TARGET_DIR"/usr/untracked_file3

}

@test "REP012: Repair modified files, new files, deleted files, and removes untracked files" {

	# repair --picky should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# repair --picky should delete any untracked files within /usr

	run sudo sh -c "$SWUPD repair --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/untracked_file3 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/contenturl -> deleted
		Inspected 21 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		  4 files found which should be deleted
		    4 of 4 files were deleted
		    0 of 4 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGET_DIR"/foo/file_1
	assert_file_not_exists "$TARGET_DIR"/bar/file_2
	assert_file_exists "$TARGET_DIR"/baz/file_3
	# untracked files
	assert_file_exists "$TARGET_DIR"/bat/untracked_file1
	assert_file_exists "$TARGET_DIR"/bar/untracked_file2
	assert_file_not_exists "$TARGET_DIR"/usr/untracked_file3
	assert_file_not_exists "$TARGET_DIR"/usr/share/defaults/swupd/versionurl
	assert_file_not_exists "$TARGET_DIR"/usr/share/defaults/swupd/contenturl

}

@test "REP013: Repair modified files, new files, deleted files, and removes untracked files from a specified location" {

	# repair --picky should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# repair --picky-tree=/bat should delete any untracked files within /bat

	run sudo sh -c "$SWUPD repair --picky --picky-tree=/bat $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/baz -> fixed
		 -> Missing file: $ABS_TARGET_DIR/baz/file_3 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/bat
		 -> Extra file: $ABS_TARGET_DIR/bat/untracked_file1 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/bat/ -> deleted
		Inspected 20 files
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
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGET_DIR"/foo/file_1
	assert_file_not_exists "$TARGET_DIR"/bar/file_2
	assert_file_exists "$TARGET_DIR"/baz/file_3
	# untracked files
	assert_file_not_exists "$TARGET_DIR"/bat/untracked_file1
	assert_file_exists "$TARGET_DIR"/bar/untracked_file2
	assert_file_exists "$TARGET_DIR"/usr/untracked_file3

}
#WEIGHT=18
