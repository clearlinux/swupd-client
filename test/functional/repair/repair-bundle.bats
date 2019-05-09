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

@test "REP006: Reapir bundle ignores not specified missing bundle" {

	# <If necessary add a detailed explanation of the test here>

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/bar/test-file2
		.fixed
		Repairing modified files
		Inspected 3 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}

@test "REP007: Repair bundle installs bundle if not installed" {

	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle3"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/baz
		.fixed
		Missing file: .*/target-dir/baz/test-file3
		.fixed
		Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle3
		.fixed
		Repairing modified files
		Inspected 3 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/baz/test-file3

}

@test "REP008: Repair bundle installs multiple bundles if not installed" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle1,test-bundle2"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/bar/test-file2
		.fixed
		Missing file: .*/target-dir/foo/test-file1
		.fixed
		Repairing modified files
		Inspected 6 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}