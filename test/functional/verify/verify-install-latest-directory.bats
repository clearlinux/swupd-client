#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -d /foo/bar "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --add-dir /foo/baz

}

@test "VER023: Verify installs a missing directory on a system based on the latest version" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install --manifest=latest"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 100
		Downloading packs for:
		 - os-core
		 - test-bundle
		Verifying files
		No extra files need to be downloaded
		Adding any missing files
		Inspected 6 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_exists "$TARGETDIR"/foo/baz

}
