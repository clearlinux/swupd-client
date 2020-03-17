#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/testfile1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/testfile2 "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle1 --header-only
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle2

	# Delete the included manifest
	sudo rm "$WEBDIR"/10/Manifest.test-bundle2
	sudo rm "$WEBDIR"/10/pack-test-bundle2-from-0.tar
	sudo rm "$WEBDIR"/10/Manifest.test-bundle2.tar

}

@test "UPD058: Report error when an included manifest is deleted" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
		Error: Failed to retrieve 10 test-bundle2 manifest
		Error: Unable to download manifest test-bundle2 version 10, exiting now
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
