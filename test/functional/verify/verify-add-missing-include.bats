#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle2 and os-core as dependency of test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 os-core
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "verify add missing include" {

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Missing file: .*/target-dir/bar
		.fixed
		Missing file: .*/target-dir/bar/test-file2
		.fixed
		Missing file: .*/target-dir/usr/share/clear/bundles/test-bundle2
		.fixed
		Fixing modified files
		Inspected 8 files
		  3 files were missing
		    3 of 3 missing files were replaced
		    0 of 3 missing files were not replaced
		  0 files found which should be deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2

}
