#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --update /core
	# remove packs so full file is used
	sudo rm -rf "$WEBDIR"/100/pack-*
	sudo rm -f "$WEBDIR"/100/delta/*
	
}

@test "update use full file" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		1 files were not in a pack
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

}
