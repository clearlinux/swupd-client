#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -n test-bundle1 -f /tracked_file,/foo/ftb1 "$TEST_NAME"  # not installed
	create_bundle -L -n test-bundle2 -f /ftb2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /ftb3 "$TEST_NAME"
	tracked_file=$(get_hash_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 /tracked_file)
	create_version -r "$TEST_NAME" 20 10
	create_version -r "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" test-bundle3 --add /tracked_file:"$TEST_NAME"/web-dir/10/files/"$tracked_file"
	update_bundle "$TEST_NAME" test-bundle2 --header-only
	add_dependency_to_manifest "$WEBDIR"/30/Manifest.test-bundle2 test-bundle1
	set_current_version "$TEST_NAME" 20

}

@test "UPD013: Update a system where a dependency that contains an already tracked file is added in the newer version" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 20 to 30
		Downloading packs for:
		 - test-bundle1
		 - os-core
		 - test-bundle2
		 - test-bundle3
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 3
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 4
		    deleted files     : 0
		No extra files need to be downloaded
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts
		Update successful. System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/tracked_file
	assert_file_exists "$TARGETDIR"/foo/ftb1
	assert_file_exists "$TARGETDIR"/ftb3

}
