#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	# remove the zero packs and fullfiles
	sudo rm "$WEBDIR"/10/pack-os-core-from-0.tar
	sudo rm -rf "$WEBDIR"/10/files/*

}

@test "INS010: An OS Install with no zero packs and no fullfile fallbacks cannot complete" {

	# no way to get content, everything should fail

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Error: zero pack downloads failed
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Error: Unable to download necessary files for this OS release
		Installation failed
	EOM
	)
	assert_is_output "$expected_output"

}
