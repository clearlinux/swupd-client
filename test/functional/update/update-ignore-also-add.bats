#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	# create the test env with two bundles (one installed)
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/testfile1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/testfile2 "$TEST_NAME"
	# create a new version 100
	create_version -p "$TEST_NAME" 100 10
	# modify test-bundle1 and test-bundle2
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/testfile1
	update_bundle "$TEST_NAME" test-bundle2 --update /bar/testfile2

	# add dependencies
	add_dependency_to_manifest -o "$WEB_DIR"/100/Manifest.test-bundle1 test-bundle2

}

@test "UPD057: Update a system where also-add dependencies are added in the newer version" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/foo/testfile1
	assert_file_not_exists "$TARGET_DIR"/bar/testfile2

}
#WEIGHT=5
