#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n upstream-bundle -f /upstream_file "$TEST_NAME"

	# create a couple 3rd-party repos within the test environment and add
	# some bundles to them
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo1

	create_bundle -n test-bundle1 -b /foo/symlink -u test-repo1 "$TEST_NAME"
}

@test "TPR072: Adding a broken symlink to a 3rd-party repo" {

	# users should be able to install bundles from 3rd-party repos

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Validating 3rd-party bundle file permissions...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_symlink_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/foo/symlink
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/test-bundle1

}
#WEIGHT=5
