#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	# create a 3rd-party bundle that has a couple of binaries
	bin_file1=$(create_file -x "$TPWEBDIR"/10/files)
	create_bundle -n test-bundle1 -f /file1,/foo/file_2,/usr/bin/bin_file1:"$bin_file1",/bin/bin_file2 -u repo1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file3,/usr/bin/bin_file1:"$bin_file1"                            -u repo1 "$TEST_NAME"
	# let's make test-bundle2 contain /bin/bin_file1 but not export it
	update_manifest "$TPWEBDIR"/10/Manifest.test-bundle2 file-status /usr/bin/bin_file1 F...

}

@test "TPR056: Adding one bundle from a third party repo that will export binaries" {

	# If a 3rd-party bundle has binaries to be exported (marked with 'x' in the
	# manifest and within /bin, /usr/bin, or /usr/local/bin) they should have a
	# script created in /opt/3rd-party/bin to make the binary available to the user

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

	# the script files should have been generated for the exported files
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/bin_file1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/bin_file2

	# verify the content of the scripts is correct
	run sudo sh -c "cat $TARGETDIR/$THIRD_PARTY_BIN_DIR/bin_file1"
	expected_output=$(cat <<-EOM
		#!/bin/bash
		export PATH=.*
		export LD_LIBRARY_PATH=.*
		$PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/repo1/usr/bin/bin_file1 .*
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "TPR057: Try adding one bundle from a third party repo that will export conflicting binaries" {

	# If a 3rd-party bundle has binaries to be exported but the binary already exists
	# in the target system, the bundle installation should be aborted

	sudo mkdir -p "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"
	sudo touch "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/bin_file1

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_COULDNT_CREATE_FILE"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		Error: There is already a binary called bin_file1 in $PATH_PREFIX/opt/3rd-party/bin
		Aborting bundle installation...
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR068: Adding bundles that share a binary" {

	# installing two bundles that share the same binary should work as long
	# as the bonary is only exported in one bundle

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"

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
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository repo1
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
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/bin_file1

	# verify the content of the scripts is correct
	run sudo sh -c "cat $TARGETDIR/$THIRD_PARTY_BIN_DIR/bin_file1"
	expected_output=$(cat <<-EOM
		#!/bin/bash
		export PATH=.*
		export LD_LIBRARY_PATH=.*
		$PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/repo1/usr/bin/bin_file1 .*
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "TPR069: Adding bundles that share a binary - reverse order" {

	# installing two bundles that share the same binary should work as long
	# as the bonary is only exported in one bundle

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle2 test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository repo1
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
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/bin_file1

	# verify the content of the scripts is correct
	run sudo sh -c "cat $TARGETDIR/$THIRD_PARTY_BIN_DIR/bin_file1"
	expected_output=$(cat <<-EOM
		#!/bin/bash
		export PATH=.*
		export LD_LIBRARY_PATH=.*
		export XDG_DATA_DIRS=.*
		export XDG_CONFIG_DIRS=.*
		$PATH_PREFIX/$THIRD_PARTY_BUNDLES_DIR/repo1/usr/bin/bin_file1 .*
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=22
