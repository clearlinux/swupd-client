#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 -d /baz/testdir "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/testdir/file_3
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_4
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/bat/file_5
	set_current_version "$TEST_NAME" 20
	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGET_DIR"/bat
	sudo touch "$TARGET_DIR"/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGET_DIR"/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/usr/untracked_file3

}

@test "REP038: Repair can be limited to a specific file" {

	# if the user uses the --file option, the repair is limited to the
	# file/directory specified by the user

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /baz/testdir/file_3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /baz/testdir/file_3
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/baz/testdir/file_3 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 1 file
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP039: Repair extra files only can be limited to a specific file" {

	# if the user uses the --file + --picky options, the repair is limited to the
	# file/directory specified by the user and looks for extra files in the
	# same path only

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /foo/file_1 --extra-files-only"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /foo/file_1
		Removing extra files under $ABS_TARGET_DIR/foo/file_1
		Inspected 0 files
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP040: Repair can fix a specific file even if its parent directory is missing" {

	# if the user uses the --file option, the repair is limited to the
	# file/directory specified by the user. If a file was specified to be
	# verified, but part of its path is missing or corrput, fix it anyway

	sudo rm -rf "$ABS_TARGET_DIR"/baz

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /baz/testdir/file_3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /baz/testdir/file_3
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/baz/testdir/file_3 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 1 file
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP041: Repair can be limited to a specific path" {

	# if the user uses the --file option, the repair is limited to the
	# file/directory specified by the user

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /bar"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/bar/bat -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/bat/file_5 -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/file_4 -> fixed
		Repairing corrupt files
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Inspected 5 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP042: Repair picky can be limited to a specific path" {

	# if the user uses the --file + --picky option, the repair is limited to the
	# file/directory specified by the user and swupd will only look for extra files
	# in the provided path

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /bar --picky"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/bar/bat -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/bat/file_5 -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/file_4 -> fixed
		Repairing corrupt files
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/bar
		 -> Extra file: $ABS_TARGET_DIR/bar/untracked_file2 -> deleted
		Inspected 6 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		  2 files found which should be deleted
		    2 of 2 files were deleted
		    0 of 2 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP043: Repair can be limited to a specific path and can look for extra files in another path" {

	# if the user uses the --file option with --picky-tree,
	# the latter should take precedence

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --file /bar --picky --picky-tree /usr"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $ABS_TARGET_DIR/bar/bat -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/bat/file_5 -> fixed
		 -> Missing file: $ABS_TARGET_DIR/bar/file_4 -> fixed
		Repairing corrupt files
		Removing extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2 -> deleted
		Removing extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/untracked_file3 -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/contenturl -> deleted
		Inspected 8 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		  4 files found which should be deleted
		    4 of 4 files were deleted
		    0 of 4 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP044: Repair can be limited to a specific path for a specific bundle" {

	# if the --bundles and --file options are used together, swupd should only
	# look in the specified path and only for files that are part of the specified
	# bundle

	write_to_protected_file -a "$ABS_TARGET_DIR"/usr/share/clear/bundles/os-core "corrupting the file"
	write_to_protected_file -a "$ABS_TARGET_DIR"/usr/share/clear/bundles/test-bundle1 "corrupting the file"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundle test-bundle1 --file /usr/share/clear/bundles"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /usr/share/clear/bundles
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/share/clear/bundles/os-core -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/share/clear/bundles/test-bundle1 -> fixed
		Removing extraneous files
		Inspected 3 files
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=40
