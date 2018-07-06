#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"

}

teardown() {

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

		File /foo was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles" {

	# for this test we need another installable bundle
	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle3"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...

		File /baz was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		..
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_in_output "$expected_output"

}

# ------------------------------------------
# Bad Cases (all bad bundles)
# ------------------------------------------

@test "bundle-add output: adding one bundle that is already added" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle2"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle2" is already installed, skipping it...
		1 bundle was already installed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding one bundle that does not exist" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, both non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle1 fake-bundle2"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle2" is invalid, skipping it...
		Warning: Bundle "fake-bundle1" is invalid, skipping it...
		Failed to install 2 of 2 bundles
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, both already added" {

	# for this test we need another bundle already installed
	create_bundle -L -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle2 test-bundle3"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is already installed, skipping it...
		Warning: Bundle "test-bundle2" is already installed, skipping it...
		2 bundles were already installed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, all invalid, one already added, one invalid" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS fake-bundle test-bundle2"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Warning: Bundle "test-bundle2" is already installed, skipping it...
		Failed to install 1 of 1 bundles
		1 bundle was already installed
	EOM
	)
	assert_in_output "$expected_output"

}

# ------------------------------------------
# Partial Cases (at least one good bundle)
# ------------------------------------------

@test "bundle-add output: adding multiple bundles, one valid, one already added" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle2" is already installed, skipping it...
		Starting download of remaining update content. This may take a while...

		File /foo was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
		1 bundle was already installed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 fake-bundle"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Starting download of remaining update content. This may take a while...

		File /foo was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Failed to install 1 of 2 bundles
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one already added, one non existent" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2 fake-bundle"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		Warning: Bundle "test-bundle2" is already installed, skipping it...
		Starting download of remaining update content. This may take a while...

		File /foo was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Failed to install 1 of 2 bundles
		1 bundle was already installed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "bundle-add output: adding multiple bundles, one valid, one with a missing file" {

	# for this test we need a bundle with a missing file so it fails when
	# trying to download the fullfile
	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"
	# remove the full file from test-bundle3
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 /baz/test-file3)
	sudo rm "$TEST_NAME"/web-dir/10/files/"$file_hash"
	sudo rm "$TEST_NAME"/web-dir/10/files/"$file_hash".tar
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle3"
	assert_status_is 18
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...

		File /baz was not in a pack
		.
		Finishing download of update content...
		Error for $file_hash tarfile extraction
	EOM
	)
	assert_in_output "$expected_output"
	assert_in_output "Starting download retry #1"
	assert_in_output "Starting download retry #2"
	assert_in_output "Starting download retry #3"
	expected_output=$(cat <<-EOM
		ERROR: Could not download some files from bundles, aborting bundle installation.
		Failed to install 2 of 2 bundles
	EOM
	)
	assert_in_output "$expected_output"

}
