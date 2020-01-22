#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n upstream-bundle -f /upstream_file "$TEST_NAME"

	# create a couple 3rd-party repos within the test environment and add
	# some bundles to them
	add_third_party_repo "$TEST_NAME" 10 1 test-repo1
	create_bundle -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"

	add_third_party_repo "$TEST_NAME" 10 1 test-repo2
	create_bundle -n test-bundle2 -f /foo/file_2 -u test-repo2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/file_3 -u test-repo2 "$TEST_NAME"

}

@test "TPR012: Adding one bundle from a third party repo" {

	# users should be able to install bundles from 3rd-party repos

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository test-repo2
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/foo/file_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TPSTATEDIR"/bundles/test-bundle2

}

@test "TPR013: Try adding one invalid bundle from a third party repo" {

	# if the user tries to add an upstream bundle using the 3rd-party command
	# the bundle should not be found/installed

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS upstream-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"

	# the same should happen with an invalid bundle
	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS fake-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle fake-bundle in the 3rd-party repositories...
		Error: bundle fake-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"
}

@test "TPR014: Try adding one valid bundle from a third party repo and one invalid" {

	# swupd should just ignore the invalid and should continue with the valid one

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS upstream-bundle test-bundle2"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository test-repo2
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/foo/file_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TPSTATEDIR"/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/usr/share/clear/bundles/upstream-bundle

	# same scenario with the arguments switched

	remove_bundle -L "$TEST_NAME"/3rd-party/test-repo2/10/Manifest.test-bundle2 test-repo2
	clean_state_dir "$TEST_NAME" test-repo2

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle2 upstream-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository test-repo2
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/foo/file_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TPSTATEDIR"/bundles/test-bundle2
}
