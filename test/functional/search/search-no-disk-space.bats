#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create a test environment with 10 MB of space
	create_test_environment -s 3 "$TEST_NAME"

	# create two bundles
	create_bundle -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 "$TEST_NAME"

	# create the state version dirs ahead of time
	sudo mkdir "$TEST_NAME"/testfs/state/10
	sudo chmod -R 0700 "$TEST_NAME"/testfs/state/10

}

@test "SRH020: Search for a bundle when there is no disk space left (downloading the MoM)" {

	# When searching for bundles, if we run out of disk space while downloading the
	# MoM we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "$SWUPD search-legacy $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Searching for 'test-bundle2'
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Failed to retrieve 10 MoM manifest
		Cannot load official manifest MoM for version 10
		Error: Failed to download manifests
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SRH021: Search for a bundle when there is no disk space left (downloading bundle manifests)" {

	# When searching for bundles, if we run out of disk space while downloading the
	# manifests we should not retry the download since it will fail for sure

	# let's replace the Manifest tar from the bundle with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/10/Manifest.test-bundle1
	sudo rm "$WEBDIR"/10/Manifest.test-bundle1.tar
	big_manifest=$(create_file "$WEBDIR"/10 3MB)
	sudo mv "$big_manifest" "$WEBDIR"/10/Manifest.test-bundle1
	sudo mv "$big_manifest".tar "$WEBDIR"/10/Manifest.test-bundle1.tar

	run sudo sh -c "$SWUPD search-legacy $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Searching for 'test-bundle2'
		Downloading Clear Linux manifests
		.* MB total...
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/10/Manifest.test-bundle1.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state\\?
		Failed to retrieve 10 test-bundle1 manifest
		Cannot load test-bundle1 sub-manifest for version 10
		1 manifest failed to download.
		Warning: One or more manifests failed to download, search results will be partial.
		Bundle test-bundle2	\\(0 MB to install\\)
		./usr/share/clear/bundles/test-bundle2
	EOM
	)
	assert_regex_is_output "$expected_output"

}
