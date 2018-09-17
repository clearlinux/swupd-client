#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"
	# Modify the filecount of the bundle manifest (and its tar) to be wrong
	manifest="$TEST_NAME"/web-dir/10/Manifest.test-bundle
	sudo sed -i "s/filecount:.*/filecount:\\t9000000/" "$manifest"
	sudo rm "$TEST_NAME"/web-dir/10/Manifest.test-bundle.tar
	create_tar "$TEST_NAME"/web-dir/10/Manifest.test-bundle

}

@test "bundle-add add bundle with invalid number of files in manifest" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"
	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Error: preposterous (9000000) number of files in test-bundle Manifest, more than 4 million skipping
		Warning: Removing corrupt Manifest.test-bundle artifacts and re-downloading...
		Error: preposterous (9000000) number of files in test-bundle Manifest, more than 4 million skipping
		Failed to load 10 test-bundle manifest
		Unable to download manifest test-bundle version 10, exiting now
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
