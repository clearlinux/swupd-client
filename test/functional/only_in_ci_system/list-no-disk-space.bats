#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	if [ -n "${SKIP_DISK_SPACE}" ]; then
		skip "Skipping disk space test"
	fi

	# create a test environment with 10 MB of space
	create_test_environment -s 10 "$TEST_NAME"

	# create two bundles, one installed one not installed (no matter its size)
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 "$TEST_NAME"

	# create the state version dirs ahead of time
	sudo mkdir -p "$ABS_MANIFEST_DIR"/10

}

@test "LST017: List local bundles when there is no disk space left (downloading the MoM)" {

	# When listing installed bundles, if we run out of disk space while downloading the
	# MoM we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
		Warning: Could not determine which installed bundles are experimental
		Installed bundles:
		 - os-core
		 - test-bundle1
		Total: 2
	EOM
	)
	assert_is_output "$expected_output"

}

@test "LST018: List all bundles when there is no disk space left (downloading the MoM)" {

	# When listing bundles, if we run out of disk space while downloading the
	# MoM we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "$SWUPD bundle-list --all $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=7
