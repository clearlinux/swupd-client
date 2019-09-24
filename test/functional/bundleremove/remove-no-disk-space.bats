#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create a test environment with 10 MB of space
	create_test_environment -s 10 "$TEST_NAME"

	# create a bundle (no matter its size since we'll be removing it)
	create_bundle -L -n test-bundle -f /file_1 "$TEST_NAME"

	# create the state version dirs ahead of time
	sudo mkdir "$TEST_NAME"/testfs/state/10

}

@test "REM017: Removing a bundle with no disk space left (downloading the MoM)" {

	# When removing a bundle, if we run out of disk space while downloading the
	# MoM we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
		Error: Unable to download/verify 10 Manifest.MoM
		Failed to remove bundle(s)
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM018: Removing a bundle with no disk space left (downloading other bundle manifests)" {

	# When removing a bundle, if we run out of disk space while downloading the
	# bundle manifests we should not retry the download since it will fail for sure
	# bundle remove needs the manifest from all other bundles in the system to check
	# for dependencies with the bundle to be removed

	# let's replace the Manifest tar from os-core with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/10/Manifest.os-core
	sudo rm "$WEBDIR"/10/Manifest.os-core.tar
	big_manifest=$(create_file "$WEBDIR"/10 15MB)
	sudo mv "$big_manifest" "$WEBDIR"/10/Manifest.os-core
	sudo mv "$big_manifest".tar "$WEBDIR"/10/Manifest.os-core.tar

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/10/Manifest.os-core.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Failed to retrieve 10 os-core manifest
		Error: Cannot load MoM sub-manifests
		Failed to remove bundle(s)
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM019: Removing a bundle with no disk space left (downloading the bundle manifest)" {

	# When removing a bundle, if we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the Manifest tar from the bundle with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/10/Manifest.test-bundle
	sudo rm "$WEBDIR"/10/Manifest.test-bundle.tar
	big_manifest=$(create_file "$WEBDIR"/10 15MB)
	sudo mv "$big_manifest" "$WEBDIR"/10/Manifest.test-bundle
	sudo mv "$big_manifest".tar "$WEBDIR"/10/Manifest.test-bundle.tar

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/10/Manifest.test-bundle.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Failed to retrieve 10 test-bundle manifest
		Error: Cannot load MoM sub-manifests
		Failed to remove bundle(s)
	EOM
	)
	assert_is_output "$expected_output"

}
