#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"
	create_bundle -L -n test-bundle4 -f /test-file4 "$TEST_NAME"

}

test_setup() {

	# do nothing, just overwrite the lib test_setup
	return

}

test_teardown() {

	# uninstall the bundle from target-dir if installed
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle1
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle2
	clean_state_dir "$TEST_NAME"

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

# ------------------------------------------
# Good Cases (all good bundles)
# ------------------------------------------

@test "bundle-add output: adding one bundle" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		..
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}


# ------------------------------------------
# Bad Cases (all bad bundles)
# ------------------------------------------

@test "bundle-add output: adding one bundle that is already added" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle3"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		1 bundle was already installed
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding one bundle that does not exist" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, both non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle1 fake-bundle2"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle2" is invalid, skipping it...
		Warning: Bundle "fake-bundle1" is invalid, skipping it...
		Failed to install 2 of 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, both already added" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle3 test-bundle4"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle4" is already installed, skipping it...
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		2 bundles were already installed
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, one already added, one invalid" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle test-bundle3"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		Failed to install 1 of 1 bundles
		1 bundle was already installed
	EOM
	)
	assert_is_output --identical "$expected_output"

}

# ------------------------------------------
# Partial Cases (at least one good bundle)
# ------------------------------------------

@test "bundle-add output: adding multiple bundles, one valid, one already added" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle3"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
		1 bundle was already installed
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 fake-bundle"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Failed to install 1 of 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one already added, one non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle3 fake-bundle"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Failed to install 1 of 2 bundles
		1 bundle was already installed
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one invalid, one already installed, one with a missing file" {


	swupd_path=$(realpath $SWUPD_DIR)
	# for this test we need a bundle with a missing file so it fails when
	# trying to download the fullfile, remove the full file from test-bundle2
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 /bar/test-file2)
	sudo rm "$TEST_NAME"/web-dir/10/files/"$file_hash"
	sudo rm "$TEST_NAME"/web-dir/10/files/"$file_hash".tar
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2 fake-bundle test-bundle3"
	assert_status_is "$EBUNDLE_INSTALL"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		.Path /bar/test-file2 is missing on the file system ... fixing
		Error: Failed to download file /bar/test-file2 in verify_fix_path
		Failed to install 3 of 3 bundles
		1 bundle was already installed
	EOM
	)
	assert_is_output "$expected_output"
}
