#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"
	sudo rm "$WEB_DIR"/10/Manifest.MoM.sig

}

@test "ADD026: Try adding a bundle without a MoM signature" {

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
	assert_file_not_exists "$TARGET_DIR"/test-file

}

@test "ADD027: Force adding a bundle without a MoM signature" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS --nosigcheck test-bundle"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: The --nosigcheck flag was used and this compromises the system security
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM WILL NOT BE VERIFIED
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/test-file

}
#WEIGHT=4
