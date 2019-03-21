#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -n test-bundle -f /file_1,/file_2 "$TEST_NAME"
	# we will make swupd believe these bundles are already installed
	# by creating their tracking file in the system
	sudo touch "$TARGETDIR"/usr/share/clear/bundles/test-bundle
	# remove the zero packs
	sudo rm "$WEBDIR"/10/pack-test-bundle-from-0.tar
	sudo rm "$WEBDIR"/10/pack-os-core-from-0.tar
	sudo rm -rf "$WEBDIR"/10/files/*

}

@test "VER040: Install bundle with no zero packs and no fullfile fallbacks" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

	# no way to get content, everything should fail
	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Downloading packs...
		Error: zero pack downloads failed
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Error: Unable to download necessary files for this OS release
		Fix did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
