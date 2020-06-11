#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /var/file "$TEST_NAME"

}

@test "INS029: Install files on /var" {

	# Making sure os-install is installing files on /var.
	# /var is filtered by heuristics, but on install time we want
	# to create them.

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 3 files
		  2 files were missing
		    2 of 2 missing files were installed
		    0 of 2 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGET_DIR"/var/file

}
