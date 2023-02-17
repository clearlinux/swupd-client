#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	if [ -n "${SKIP_DISK_SPACE}" ]; then
		skip "Skipping disk space test"
	fi

	# create a test environment with 10 MB of space
	create_test_environment -s 3 "$TEST_NAME"

	# create two bundles
	create_bundle -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 "$TEST_NAME"

	# create the state version dirs ahead of time
	sudo mkdir -p "$ABS_MANIFEST_DIR"/10

}

@test "SRH020: Search for a bundle when there is no disk space left (downloading the MoM)" {

	# When searching for bundles, if we run out of disk space while downloading the
	# MoM we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SRH021: Search for a bundle when there is no disk space left (downloading bundle manifests)" {

	# When searching for bundles, if we run out of disk space while downloading the
	# manifests we should not retry the download since it will fail for sure

	# let's replace the Manifest tar from the bundle with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle1
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle1.tar
	big_manifest=$(create_file "$WEB_DIR"/10 3MB)
	sudo mv "$big_manifest" "$WEB_DIR"/10/Manifest.test-bundle1
	sudo mv "$big_manifest".tar "$WEB_DIR"/10/Manifest.test-bundle1.tar

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.test-bundle1.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state\\?
		Error: Failed to retrieve 10 test-bundle1 manifest
		Error: Cannot load test-bundle1 manifest for version 10
		Warning: Failed to download 1 manifest - search results will be partial
		Searching for 'test-bundle2'
		Bundle test-bundle2 \\(0 MB to install\\)
		./usr/share/clear/bundles/test-bundle2
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=6
