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

@test "DIA013: Diagnose a system that has a missing also-add" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		.* Missing file: .*/target-dir/foo
		.* Missing file: .*/target-dir/foo/test-file1
		.* Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle1
		Checking for corrupt files
		Checking for extraneous files
		Inspected 5 files
		  3 files were missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful

	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "DIA014: Diagnose a system after a bundle-remove of an also-add" {

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

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Inspected 5 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"
}
#WEIGHT=7
