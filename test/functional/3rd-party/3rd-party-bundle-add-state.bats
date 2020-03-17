#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -n test-bundle1 -f /foo/file_1,/home/testdir/file_2,/etc/file_3,/boot/file_4 -u repo1 "$TEST_NAME"

}

@test "TPR073: Adding a third party bundle with state files" {

	# If a 3rd-party bundle has state files, they should be installed

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/foo/file_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/home/testdir/file_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/etc/file_3
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/boot/file_4

}
