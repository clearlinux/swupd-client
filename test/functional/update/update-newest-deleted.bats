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

@test "update where the newest version of a file was deleted" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 30
		Downloading packs...
		Statistics for going from version 10 to version 30:
		    changed bundles   : 3
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 1
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		1 files were not in a pack
		Update successful. System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/testfile

}

