#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /testfile1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /testfile2 "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle1 --update /testfile1
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle2

}

@test "update include a bundle from an older release" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Extracting test-bundle1 pack for version 100
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 2
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
	assert_is_output "$expected_output"
	# changed files
	assert_file_exists "$TARGETDIR"/testfile1
	# new file
	assert_file_exists "$TARGETDIR"/testfile2

}

