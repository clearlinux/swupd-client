#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/test-file3 "$TEST_NAME"

	# Create bundle dependencies
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.os-core test-bundle1
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.os-core test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3

}

@test "REP031: Repairs a system that has a missing also-add" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		.* Missing file: .*/target-dir/foo -> fixed
		.* Missing file: .*/target-dir/foo/test-file1 -> fixed
		.* Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle1 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 5 files
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
	assert_file_not_exists "$TARGETDIR"/bar/test-file2

}

@test "REP032: Repairs a system after removing an also-add" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file3

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3"
	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file3

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 5 files
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file3

}
