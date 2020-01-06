#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/bar/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -d /foo,/foo/bar "$TEST_NAME"
	# bundles created with the testlib add all needed directories to the
	# manifest by default, so we need to remove the directory from test-bundle1
	# so its missing the path to the file.
	remove_from_manifest "$WEBDIR"/10/Manifest.test-bundle1 /foo
	remove_from_manifest "$WEBDIR"/10/Manifest.test-bundle1 /foo/bar
	# since test-bundle2 is already installed, both directories defined
	# there already exist, so we need to delete one of the /foo/bar so it
	# can be fixed using verify_fix_path
	sudo rm -rf "$TARGETDIR"/foo/bar

}

@test "ADD022: When adding a bundle and a path is missing on the fs, bundle-add fixes it" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		 -> Missing directory: $PATH_PREFIX/foo/bar -> fixed
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/bar/test-file1

}
