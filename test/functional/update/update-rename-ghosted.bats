#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/test-file1,/bar/test-file2,/test-file3 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --rename-legacy /foo/test-file1 /foo/test-file1-renamed
	update_bundle "$TEST_NAME" test-bundle --rename-legacy /bar/test-file2 /bar/test-file2-renamed
	update_bundle "$TEST_NAME" test-bundle --update /test-file3
	update_manifest "$WEBDIR"/20/Manifest.test-bundle file-status /foo/test-file1 .g.r
	update_manifest "$WEBDIR"/20/Manifest.test-bundle file-status /bar/test-file2 .g.r

}

@test "update rename ghosted file" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting test-bundle pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 2
		    deleted files     : 2
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
	# all the files should still exist, including the ghosted ones
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/foo/test-file1-renamed
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/bar/test-file2-renamed

}

