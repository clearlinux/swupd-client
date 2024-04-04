#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	add_os_core_update_bundle "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/bat/file_3

	# force some repairs in the target system
	set_current_version "$TEST_NAME" 20
	sudo touch "$TARGET_DIR"/usr/untracked_file

	# force failures while repairing
	sudo touch "$TARGET_DIR"/baz
	sudo chattr +i "$TARGET_DIR"/baz
	sudo chattr +i "$TARGET_DIR"/usr/untracked_file

}

test_teardown() {

	sudo chattr -i "$TARGET_DIR"/usr/untracked_file
	sudo chattr -i "$TARGET_DIR"/baz

}

@test "REP045: Repair failures" {

	# Make sure the output is useful when repair fails

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky"

	assert_status_is_not "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		 -> Missing file: $ABS_TARGET_DIR/baz/bat -> not fixed
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		 -> Missing file: $ABS_TARGET_DIR/baz/bat/file_3 -> not fixed
		Repairing corrupt files
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		 -> Hash mismatch for file: $ABS_TARGET_DIR/baz -> not fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/foo/file_1 -> fixed
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/lib/os-release -> fixed
		The removal of extraneous files will be skipped due to the previous errors found repairing
		Removing extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/untracked_file -> not deleted (Operation not permitted)
		Inspected 23 files
		  2 files were missing
		    0 of 2 missing files were replaced
		    2 of 2 missing files were not replaced
		  3 files did not match
		    2 of 3 files were repaired
		    1 of 3 files were not repaired
		  1 file found which should be deleted
		    0 of 1 files were deleted
		    1 of 1 files were not deleted
		Calling post-update helper scripts
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP046: repair failure using --quiet" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky --quiet"

	assert_status_is_not "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		$ABS_TARGET_DIR/baz/bat -> not fixed
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		$ABS_TARGET_DIR/baz/bat/file_3 -> not fixed
		Error: Target has different file type but could not be moved to backup location: $ABS_TARGET_DIR/baz
		$ABS_TARGET_DIR/baz -> not fixed
		$ABS_TARGET_DIR/foo/file_1 -> fixed
		$ABS_TARGET_DIR/usr/lib/os-release -> fixed
		$ABS_TARGET_DIR/usr/untracked_file -> not deleted (Operation not permitted)
	EOM
	)
	assert_is_output "$expected_output"

}

#WEIGHT=6
