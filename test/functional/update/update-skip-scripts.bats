#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --update /core
	update_bundle "$TEST_NAME" os-core --add /testfile

}

@test "update skip post-update scripts" {

	run sudo sh -c "$SWUPD update --no-scripts $SWUPD_OPTS"
	# Should still be successful
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Extracting os-core pack for version 100
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 1
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		WARNING: post-update helper scripts skipped due to --no-scripts argument
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	# The file should still be added
	assert_file_exists "$TARGETDIR"/testfile
}
