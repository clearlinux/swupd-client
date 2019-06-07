#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle2 and os-core as dependency of test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 os-core
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "REP001: Repairs a system that has a missing dependency" {

	# <If necessary add a detailed explanation of the test here>

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		.* Missing file: .*/target-dir/bar -> fixed
		.* Missing file: .*/target-dir/bar/test-file2 -> fixed
		.* Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle2 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 8 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}
