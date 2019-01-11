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
	file1=/test-file:"$(create_file "$WEBDIR"/20/files 20MB)"
	update_bundle "$TEST_NAME" test-bundle --add "$file1"

	# create the state version dirs ahead of time
	sudo mkdir "$TEST_NAME"/testfs/state/10
	sudo mkdir "$TEST_NAME"/testfs/state/20

}

@test "UPD045: Updating a system with no disk space left (downloading the current MoM)" {

	# When updating a system and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$EMOM_LOAD"
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Curl: Error downloading to local file - $TEST_DIRNAME/testfs/state/10/Manifest.MoM.tar
		Check free space for $TEST_DIRNAME/testfs/state?
		Failed to retrieve 10 MoM manifest
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD046: Updating a system with no disk space left (downloading the server MoM)" {

	# When updating a system and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/20/Manifest.MoM
	sudo rm "$WEBDIR"/20/Manifest.MoM.tar
	fake_manifest=$(create_file "$WEBDIR"/20 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEBDIR"/20/"$manifest_name" "$WEBDIR"/20/Manifest.MoM
	sudo mv "$WEBDIR"/20/"$manifest_name".tar "$WEBDIR"/20/Manifest.MoM.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$EMOM_LOAD"
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Curl: Error downloading to local file - $TEST_DIRNAME/testfs/state/20/Manifest.MoM.tar
		Check free space for $TEST_DIRNAME/testfs/state?
		Failed to retrieve 20 MoM manifest
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD047: Updating a system with no disk space left (downloading the current bundle manifests)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/10/Manifest.test-bundle
	sudo rm "$WEBDIR"/10/Manifest.test-bundle.tar
	fake_manifest=$(create_file "$WEBDIR"/10 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEBDIR"/10/"$manifest_name" "$WEBDIR"/10/Manifest.test-bundle
	sudo mv "$WEBDIR"/10/"$manifest_name".tar "$WEBDIR"/10/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$ERECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Curl: Error downloading to local file - $TEST_DIRNAME/testfs/state/10/Manifest.test-bundle.tar
		Check free space for $TEST_DIRNAME/testfs/state?
		Failed to retrieve 10 test-bundle manifest
		Cannot load current MoM sub-manifests, exiting
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD048: Updating a system with no disk space left (downloading the server bundle manifests)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/20/Manifest.test-bundle
	sudo rm "$WEBDIR"/20/Manifest.test-bundle.tar
	fake_manifest=$(create_file "$WEBDIR"/20 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEBDIR"/20/"$manifest_name" "$WEBDIR"/20/Manifest.test-bundle
	sudo mv "$WEBDIR"/20/"$manifest_name".tar "$WEBDIR"/20/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$ERECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Curl: Error downloading to local file - $TEST_DIRNAME/testfs/state/20/Manifest.test-bundle.tar
		Check free space for $TEST_DIRNAME/testfs/state?
		Failed to retrieve 20 test-bundle manifest
		Unable to download manifest test-bundle version 20, exiting now
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD049: Updating a system with no disk space left (downloading the bundle files)" {

	# When updating a system and we run out of disk space while downloading the
	# bundle files we should not retry the download since it will fail for sure

	file_hash=$(get_hash_from_manifest "$WEBDIR"/20/Manifest.test-bundle /test-file)

	run sudo sh -c "timeout 30 $SWUPD update $SWUPD_OPTS"

	assert_status_is "$EFILEDOWNLOAD"
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Error for $TEST_DIRNAME/testfs/state/pack-test-bundle-from-10-to-20.tar download: Response 0 - Failed writing received data to disk/application
		Check free space for $TEST_DIRNAME/testfs/state?
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Error for $TEST_DIRNAME/testfs/state/download/.$file_hash.tar download: Response 0 - Failed writing received data to disk/application
		Check free space for $TEST_DIRNAME/testfs/state?
		ERROR: Could not download all files, aborting update
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}
