#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

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
#WEIGHT=3
