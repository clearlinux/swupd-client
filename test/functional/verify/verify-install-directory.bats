#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -d /foo/bar "$TEST_NAME"
	# remove the directory bar from the target-dir
	sudo rm -rf "$TARGETDIR"/foo/bar

}

@test "verify install directory" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Downloading packs...
		Extracting os-core pack for version 10
		Extracting test-bundle pack for version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Inspected 5 files
		  1 files were missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_exists "$TARGETDIR"/foo/bar

}
