#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/bar/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -d /foo,/foo/bar "$TEST_NAME"
	# bundles created with the testlib add all needed directories to the
	# manifest by default, so we need to remove the directory from test-bundle1
	# so its missing the path to the file.
	remove_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 /foo
	remove_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 /foo/bar
	# since test-bundle2 is already installed, both directories defined
	# there already exist, so we need to delete one of the /foo/bar so it
	# can be fixed using verify_fix_path
	sudo rm -rf "$TEST_NAME"/target-dir/foo/bar

}

@test "bundle-add verify_fix_path support" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Update target directory does not exist: $TEST_DIRNAME/target-dir/foo/bar. Auto-fix disabled
		Path /foo/bar is missing on the file system ... fixing
		Path /foo/bar/test-file1 is missing on the file system ... fixing
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
