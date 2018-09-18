#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/testfile1,/foo/testfile3 "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --rename /foo/testfile3 /foo/testfile2

}

@test "update simple rename" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Extracting test-bundle pack for version 100
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 1
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/testfile2
	assert_file_not_exists "$TARGETDIR"/foo/testfile3

}
