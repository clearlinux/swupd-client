#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	create_bundle -n test-bundle -f /file_1,/file_2 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle os-core

}

@test "INS008: Install OS specifying a bundle with invalid name" {

	# Users should be warned if they provide an invalid bundle name when
	# installing a OS

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --bundles bad-name,test-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Warning: Bundle "bad-name" is invalid or no longer available
		Downloading missing manifests...
		Error: One or more of the provided bundles are not available at version 10
		Please make sure the name of the provided bundles are correct, or use --force to override
		Installation failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "INS009: An OS install with non fatal errors can be forced to continue" {

	# users should be able to use --force option to continue with the OS
	# install when non fatal error occurrs

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --force --bundles bad-name,test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Warning: Bundle "bad-name" is invalid or no longer available
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		 - test-bundle
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 5 files
		  5 files were missing
		    5 of 5 missing files were installed
		    0 of 5 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle
	assert_file_exists "$TARGETDIR"/file_1
	assert_file_exists "$TARGETDIR"/file_2

}
