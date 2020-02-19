#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	create_bundle -n test-bundle1 -f /file_1,/file_2 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo/file_3 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/file_4 "$TEST_NAME"
	# os-core is always included by all manifests
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 os-core
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 os-core
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 os-core

}

@test "INS004: An OS install can include user specified bundles during OS install" {

	# users can specify bundles to be added to the OS during install,
	# os-core does not need to be specified, it will be installed by default

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --bundles test-bundle2,test-bundle3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		 - test-bundle2
		 - test-bundle3
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 8 files
		  8 files were missing
		    8 of 8 missing files were installed
		    0 of 8 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/file_3
	assert_file_exists "$TARGETDIR"/bar/file_4
	assert_file_exists "$TARGETDIR"/var/lib/swupd/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/var/lib/swupd/bundles/test-bundle3
	# make sure non specified bundles were not installed
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/file_1
	assert_file_not_exists "$TARGETDIR"/file_2
	assert_file_not_exists "$TARGETDIR"/var/lib/swupd/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/var/lib/swupd/bundles/os-core

}
#WEIGHT=4
