#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /testfile "$TEST_NAME"
	create_bundle -L -n test-bundle2 -d /foo "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --update /core
	update_bundle "$TEST_NAME" test-bundle1 --delete /testfile
	update_bundle "$TEST_NAME" test-bundle2 --add /testfile
	create_version -p "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" os-core --update /core
	update_bundle "$TEST_NAME" test-bundle2 --delete /testfile

}

@test "UPD003: Updating a system where a file was deleted in the newer version" {

	# NOTE: we don't create delta packs when a file is deleted in an update with the test library
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 30
		Downloading packs for:
		 - os-core
		 - test-bundle1
		 - test-bundle2
		Finishing packs extraction...
		Statistics for going from version 10 to version 30:
		    changed bundles   : 3
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 1
		No extra files need to be downloaded
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts
		Update successful. System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/testfile

}

