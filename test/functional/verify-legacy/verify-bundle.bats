#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/bar/test-file2

}

@test "VER001: Verify bundle ignores not specified missing bundle" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --bundles test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --fix option has been superseded
		Please consider using "swupd repair" instead
		Verifying version 10
		Limiting verify to the following bundles:
		 - test-bundle2
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/bar/test-file2 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 3 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}

@test "VER002: Verify bundle installs bundle if not installed" {

	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --bundles test-bundle3"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --fix option has been superseded
		Please consider using "swupd repair" instead
		Verifying version 10
		Limiting verify to the following bundles:
		 - test-bundle3
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/baz -> fixed
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/baz/test-file3 -> fixed
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/usr/share/clear/bundles/test-bundle3 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 3 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/baz/test-file3

}

@test "VER003: Verify bundle installs multiple bundles if not installed" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --bundles test-bundle1,test-bundle2"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --fix option has been superseded
		Please consider using "swupd repair" instead
		Verifying version 10
		Limiting verify to the following bundles:
		 - test-bundle1
		 - test-bundle2
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/bar/test-file2 -> fixed
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/foo/test-file1 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 6 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}

@test "VER004: Verify bundle warns users about possible undesired side effects when using --bundles + --extra-files-only" {

	# with --extra-files-only
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --extra-files-only --bundles test-bundle1"
	expected_output=$(cat <<-EOM
		Warning: Using the --bundles option with --extra-files-only may have some undesired side effects
		verify will remove all files in /usr that are not part of --bundles unless listed in the --picky-whitelist
	EOM
	)
	assert_in_output "$expected_output"

}

@test "VER005: Verify bundle warns users about possible undesired side effects when using --bundles + --picky" {

	# with --picky
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --picky --bundles test-bundle1"
	expected_output=$(cat <<-EOM
		Warning: Using the --bundles option with --picky may have some undesired side effects
		verify will remove all files in /usr that are not part of --bundles unless listed in the --picky-whitelist
	EOM
	)
	assert_in_output "$expected_output"

}
