#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle-1 -f /test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle-2 -f /test-file2,/test-file3 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle-1 test-bundle-2
	create_version "$TEST_NAME" 20 10
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /test-file1
	update_bundle -i "$TEST_NAME" test-bundle-2 --update /test-file2

}

@test "update works correctly using an iterative manifest that has an old dependency" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting test-bundle-2 pack for version 20
		Extracting test-bundle-1 pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
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

}

