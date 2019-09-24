#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create a test environment with 10 MB of space
	create_test_environment -s 10 "$TEST_NAME"

	# create a bundle with 10+ files
	bfiles="/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10"
	create_bundle -L -n test-bundle -f "$bfiles" "$TEST_NAME"

	# create a new version
	create_version "$TEST_NAME" 20 10

	# create a file with a size of 20 MB and create an
	# update for the bundle that uses that file
	file1=/test-file:"$(create_file "$WEBDIR"/10/files 20MB)"
	update_bundle "$TEST_NAME" test-bundle --add "$file1"

	# create the state version dirs ahead of time
	sudo mkdir "$TEST_NAME"/testfs/state/10
	sudo mkdir "$TEST_NAME"/testfs/state/20

}

@test "REP022: Repair a system with no disk space left (downloading the MoM)" {

	# When verifying a system and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "timeout 30 $SWUPD repair --force --version=20 $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/20/Manifest.MoM.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Failed to retrieve 20 MoM manifest
		Error: Unable to download/verify 20 Manifest.MoM
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP023: Repair a system with no disk space left (downloading the bundle manifest)" {

	# When verifying a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEBDIR"/20/Manifest.test-bundle
	sudo rm "$WEBDIR"/20/Manifest.test-bundle.tar
	big_manifest=$(create_file "$WEBDIR"/20 15MB)
	sudo mv "$big_manifest" "$WEBDIR"/20/Manifest.test-bundle
	sudo mv "$big_manifest".tar "$WEBDIR"/20/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD repair --force --version=20 $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MANIFEST"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Warning: The --force option is specified; ignoring version mismatch for repair
		Downloading missing manifests...
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/20/Manifest.test-bundle.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Failed to retrieve 20 test-bundle manifest
		Error: Unable to download manifest test-bundle version 20, exiting now
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP024: Repair a system with no disk space left (downloading the bundle files)" {

	# When verifying a system and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	fhash=$(get_hash_from_manifest "$WEBDIR"/20/Manifest.test-bundle /test-file)

	run sudo sh -c "timeout 30 $SWUPD repair --force --version=20 $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Warning: The --force option is specified; ignoring version mismatch for repair
		Downloading missing manifests...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Error: Curl - Error downloading to local file - 'file://$TEST_DIRNAME/web-dir/20/files/$fhash.tar'
		Error: Curl - Check free space for $TEST_DIRNAME/testfs/state?
		Error: Unable to download necessary files for this OS release
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
