#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a couple of 3rd-party repos with bundles
	add_third_party_repo "$TEST_NAME" 10 staging repo1
	create_bundle -n test-bundle1 -f /foo/file_1A -u repo1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/file_2 -u repo1 "$TEST_NAME"

	add_third_party_repo "$TEST_NAME" 10 staging repo2
	create_bundle -n test-bundle1 -f /baz/file_1B -u repo2 "$TEST_NAME"

}

@test "TPR016: Try adding a bundle that exists in many third party repos" {

	# when a bundle is in many 3rd-party repos the user needs to specify
	# the repo using the --repo flag or the installation won't continue

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundle test-bundle1 found in 3rd-party repository repo2
		Error: bundle test-bundle1 was found in more than one 3rd-party repository
		Please specify a repository using the --repo flag
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/foo/file_1A
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo2/baz/file_1B
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo2/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/3rd-party/repo1/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/3rd-party/repo2/bundles/test-bundle1

}

@test "TPR017: Adding a bundle that exists in many third party repos specifying the repo" {

	# when a bundle is in many 3rd-party repos the user needs to specify
	# the repo using the --repo flag or the installation won't continue

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1 --repo repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
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
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/foo/file_1A
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo2/baz/file_1B
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo2/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/3rd-party/repo1/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/3rd-party/repo2/bundles/test-bundle1

}
