#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	# remove the zero pack
	sudo rm "$WEBDIR"/10/pack-os-core-from-0.tar

}

@test "INS011: An OS install should fall back to use fullfiles if no packs are available" {

	# install should use packs if available as first choice, but if no packs exist
	# for the content to be installed, then swupd should fall back to using fullfiles

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Error: zero pack downloads failed
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing base OS and selected bundles
		Inspected 2 files
		  2 files were missing
		    2 of 2 missing files were installed
		    0 of 2 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core

}
#WEIGHT=1
