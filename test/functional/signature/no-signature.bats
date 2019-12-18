#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"
	sudo rm "$WEBDIR"/10/Manifest.MoM.sig

}

@test "SIG007: Swupd bundle-add without a MoM signature" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Error: Signature verification failed for manifest version 10
		Error: Cannot load official manifest MoM for version 10
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG008: Force swupd bundle-add a bundle without a MoM signature" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS --nosigcheck test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --nosigcheck flag was used, THE MANIFEST SIGNATURE WILL NOT BE VERIFIED
		Be aware that this compromises the system security
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
