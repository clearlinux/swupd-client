#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /test-file2 "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle1 --header-only
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle2
	set_current_version "$TEST_NAME" 100

}

@test "REP002: Repair a system that has a missing dependency from a previous version" {

	# <If necessary add a detailed explanation of the test here>

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 100
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		.* Missing file: .*/target-dir/test-file2 -> fixed
		.* Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle2 -> fixed
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
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2

}
#WEIGHT=4
