#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /test-file2 "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle1 --header-only
	add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle2
	set_current_version "$TEST_NAME" 100

}

@test "verify add missing old include" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 100
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Missing file: .*/target-dir/test-file2
		.fixed
		Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle2
		.fixed
		Fixing modified files
		Inspected 6 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  0 files found which should be deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2

}
