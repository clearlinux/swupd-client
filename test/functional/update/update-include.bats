#!/usr/bin/env bats

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
	# create new bundles for version 100
	create_bundle -v 100 -n test-bundle3 -f /foo/testfile3 "$TEST_NAME"
	create_bundle -v 100 -n test-bundle4 -f /foo/testfile4 "$TEST_NAME"
	create_bundle -v 100 -n test-bundle5 -f /foo/testfile5 "$TEST_NAME"
	create_bundle -v 100 -n test-bundle6 -f /foo/testfile6 "$TEST_NAME"
	create_bundle -v 100 -n test-bundle7 -f /foo/testfile7 "$TEST_NAME"
	# add dependencies
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle3
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle2 test-bundle7
	add_dependency_to_manifest -p "$WEBDIR"/100/Manifest.test-bundle3 test-bundle2
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle3 test-bundle4
	add_dependency_to_manifest -p "$WEBDIR"/100/Manifest.test-bundle4 test-bundle5
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle4 test-bundle6
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle5 test-bundle2
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle6 os-core
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle7 os-core


}

@test "update with includes" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Extracting test-bundle3 pack for version 100
		Extracting test-bundle4 pack for version 100
		Extracting test-bundle6 pack for version 100
		Extracting test-bundle5 pack for version 100
		Extracting test-bundle2 pack for version 100
		Extracting test-bundle7 pack for version 100
		Extracting test-bundle1 pack for version 100
		Couldn.t use delta file .* no .from. file to apply was found
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 6
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 13
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		2 files were not in a pack
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_regex_is_output "$expected_output"
	# changed files
	assert_file_exists "$TARGETDIR"/foo/testfile1
	assert_file_exists "$TARGETDIR"/bar/testfile2
	# new files
	assert_file_exists "$TARGETDIR"/foo/testfile3
	assert_file_exists "$TARGETDIR"/foo/testfile4
	assert_file_exists "$TARGETDIR"/foo/testfile5
	assert_file_exists "$TARGETDIR"/foo/testfile6
	assert_file_exists "$TARGETDIR"/foo/testfile7

}
