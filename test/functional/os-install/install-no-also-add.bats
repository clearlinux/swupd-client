#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/test-file3 "$TEST_NAME"

	# Create bundle dependencies
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.os-core test-bundle1
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.os-core test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3

}

@test "INS016: Install without also-add bundles" {

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH --path $TARGETDIR --skip-optional"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - test-bundle1
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 5 files
		  3 files were missing
		    3 of 3 missing files were installed
		    0 of 3 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bar/test-file3

}
#WEIGHT=4
