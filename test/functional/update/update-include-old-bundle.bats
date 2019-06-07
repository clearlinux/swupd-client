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

@test "UPD012: Update a system where an old dependency is added in the newer version" {

	# the "old" dependency is one that was not updated in the last version

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs for:
		 - test-bundle2
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 2
		    deleted files     : 0
		No extra files need to be downloaded
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	# changed files
	assert_file_exists "$TARGETDIR"/testfile1
	# new file
	assert_file_exists "$TARGETDIR"/testfile2

}

