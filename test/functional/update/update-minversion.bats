#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -L -n bundle1 -v 10 -f /file1 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10 staging
	update_minversion 20

	sudo rm -f "$WEBDIR"/20/files/*
}

@test "skip fullfile download for unchanged file in minversion update " {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 4
		    deleted files     : 0
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 20
	EOM
	)
	assert_in_output "$expected_output"
}
