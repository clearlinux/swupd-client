#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -d /foo/bar "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --add /bat/baz/test-file
	# remove the directory bar from the target-dir
	sudo rm -rf "$TARGETDIR"/foo/bar

}

@test "VER022: Verify installs missing content on a system based on a specific version" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install --manifest=20"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 20
		Downloading packs for:
		 - os-core
		 - test-bundle
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Inspected 8 files
		  4 files were missing
		    4 of 4 missing files were replaced
		    0 of 4 missing files were not replaced
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_exists "$TARGETDIR"/foo/bar
	assert_file_exists "$TARGETDIR"/bat/baz/test-file

}
