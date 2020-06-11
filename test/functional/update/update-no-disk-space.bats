#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create a test environment with 10 MB of space and a
	# bundle "test-bundle" installed on it
	create_test_environment -s 10 "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo "$TEST_NAME"

	# create a new version 20
	create_version "$TEST_NAME" 20 10

	# create a file with a size of 20 MB and create an update
	# for test-bundle that adds that file
	file1=/test-file:"$(create_file "$WEB_DIR"/20/files 20MB)"
	update_bundle "$TEST_NAME" test-bundle --add "$file1"

	# create the state version dirs ahead of time
	sudo mkdir -p "$ABS_MANIFEST_DIR"/10
	sudo mkdir -p "$ABS_MANIFEST_DIR"/20

}

@test "UPD045: Updating a system with no disk space left (downloading the current MoM)" {

	# When updating a system and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD046: Updating a system with no disk space left (downloading the server MoM)" {

	# When updating a system and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEB_DIR"/20/Manifest.MoM
	sudo rm "$WEB_DIR"/20/Manifest.MoM.tar
	fake_manifest=$(create_file "$WEB_DIR"/20 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEB_DIR"/20/"$manifest_name" "$WEB_DIR"/20/Manifest.MoM
	sudo mv "$WEB_DIR"/20/"$manifest_name".tar "$WEB_DIR"/20/Manifest.MoM.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/20/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 20 MoM manifest
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD047: Updating a system with no disk space left (downloading the current bundle manifests)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle.tar
	fake_manifest=$(create_file "$WEB_DIR"/10 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEB_DIR"/10/"$manifest_name" "$WEB_DIR"/10/Manifest.test-bundle
	sudo mv "$WEB_DIR"/10/"$manifest_name".tar "$WEB_DIR"/10/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.test-bundle.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 test-bundle manifest
		Error: Cannot load current MoM sub-manifests, exiting
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD048: Updating a system with no disk space left (downloading the server bundle manifests)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEB_DIR"/20/Manifest.test-bundle
	sudo rm "$WEB_DIR"/20/Manifest.test-bundle.tar
	fake_manifest=$(create_file "$WEB_DIR"/20 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEB_DIR"/20/"$manifest_name" "$WEB_DIR"/20/Manifest.test-bundle
	sudo mv "$WEB_DIR"/20/"$manifest_name".tar "$WEB_DIR"/20/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/20/Manifest.test-bundle.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 20 test-bundle manifest
		Error: Unable to download manifest test-bundle version 20, exiting now
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD049: Updating a system with no disk space left (downloading the bundle files)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle files we should not retry the download since it will fail for sure

	file_hash=$(get_hash_from_manifest "$WEB_DIR"/20/Manifest.test-bundle /test-file)

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output1=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs \\(20.*\\) for:
	EOM
	)
	expected_output2=$(cat <<-EOM
		 - test-bundle
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/20/pack-test-bundle-from-10.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/20/files/$file_hash.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Could not download all files, aborting update
		Update failed
	EOM
	)
	assert_regex_in_output "$expected_output1"
	assert_in_output "$expected_output2"

}
#WEIGHT=17
