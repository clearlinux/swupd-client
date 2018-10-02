#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -L -n bundle1 -v 10 -f /file1,/file2,/file3 "$TEST_NAME"

	sudo sed -i "/^minversion:\\t/d" "$WEBDIR"/10/Manifest.MoM

	create_version "$TEST_NAME" 20 10 staging

	update_bundle -p "$TEST_NAME" bundle1 --add /file4
	update_bundle -p "$TEST_NAME" bundle1 --update /file1
	update_bundle "$TEST_NAME" bundle1 --delete /file2
}

@test "Update when missing minversion header" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting bundle1 pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    deleted files     : 1
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

	assert_file_exists "$TARGETDIR"/file1
	assert_file_exists "$TARGETDIR"/file3
	assert_file_exists "$TARGETDIR"/file4

	assert_file_not_exists "$TARGETDIR"/file2
}
