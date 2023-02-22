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

	# create a file with a size of 20 MB and create a bundle
	# that uses that file and 10 other files (to force pack download)
	file1=/test-file:"$(create_file "$WEB_DIR"/10/files 20MB)"
	bfiles="/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10"
	create_bundle -n test-bundle -f "$bfiles","$file1" "$TEST_NAME"

	# create the state dirs ahead of time (there will be no disk space later)
	sudo mkdir -p "$ABS_MANIFEST_DIR"/10

}

@test "ADD044: Adding a bundle with no disk space left (downloading the MoM)" {

	# When adding a bundle and we run out of disk space while downloading the
	# MoMs we should not retry the download since it will fail for sure

	# fill up all the space in the disk
	sudo dd if=/dev/zero of="$TEST_NAME"/testfs/dummy >& /dev/null || print "Using all space left in disk"

	run sudo sh -c "timeout 30 $SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 MoM manifest
		Error: Cannot load official manifest MoM for version 10
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD045: Adding a bundle with no disk space left (downloading the bundle manifest)" {

	# When adding a bundle and we run out of disk space while downloading the
	# bundle manifest we should not retry the download since it will fail for sure

	# let's replace the bundle Manifest tar with a much larger file that will exceed
	# the available space on disk
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle
	sudo rm "$WEB_DIR"/10/Manifest.test-bundle.tar
	fake_manifest=$(create_file "$WEB_DIR"/10 15MB)
	manifest_name=$(basename "$fake_manifest")
	sudo mv "$WEB_DIR"/10/"$manifest_name" "$WEB_DIR"/10/Manifest.test-bundle
	sudo mv "$WEB_DIR"/10/"$manifest_name".tar "$WEB_DIR"/10/Manifest.test-bundle.tar

	run sudo sh -c "timeout 30 $SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Error: Curl - Error downloading to local file - 'file://$ABS_TEST_DIR/web-dir/10/Manifest.test-bundle.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state?
		Error: Failed to retrieve 10 test-bundle manifest
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD046: Adding a bundle with no disk space left (downloading the bundle files)" {

	# When adding a bundle, the default is to check the disk space available
	# and skip the installation if it is determined there is no enough space

	run sudo sh -c "timeout 30 $SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_DISK_SPACE_ERROR"
	expected_output=$(cat <<-EOM
		Loading required manifests\.\.\.
		Error: Bundle too large by [0-9]+M
		NOTE: currently, swupd only checks /usr/ \(or the passed-in path with /usr/ appended\) for available space
		To skip this error and install anyways, add the --skip-diskspace-check flag to your command
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "ADD047: Adding a bundle with no disk space left and skipping diskspace check (downloading the bundle files)" {

	# When adding a bundle, the default is to check the disk space available
	# and skip the installation if it is determined there is no enough space, however
	# this check can be skipped with a flag, and in that case if we run out of disk
	# space while downloading the bundle files we should not retry the download since
	# it will fail for sure

	# use a web server for serving the content, this is necessary
	# since the code behaves differently if the content is local (e.g. file://)
	start_web_server -r
	set_upstream_server "$TEST_NAME" http://localhost:"$(get_web_server_port "$TEST_NAME")"/"$TEST_NAME"/web-dir

	run sudo sh -c "timeout 30 $SWUPD bundle-add --skip-diskspace-check --allow-insecure-http $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output1=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Loading required manifests...
		Downloading packs \\(.* MB\\) for:
		 - test-bundle
		Error: Curl - Error downloading to local file - 'http://localhost:$(get_web_server_port "$TEST_NAME")/$TEST_NAME/web-dir/10/pack-test-bundle-from-0.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state\\?
		Finishing packs extraction...
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Error: Curl - Error downloading to local file - 'http://localhost:$(get_web_server_port "$TEST_NAME")/$TEST_NAME/web-dir/10/.*.tar'
		Error: Curl - Check free space for $ABS_TEST_DIR/testfs/state\\?
	EOM
	)
	expected_output2=$(cat <<-EOM
		Error: Could not download some files from bundles, aborting bundle installation
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_regex_in_output "$expected_output1"
	assert_in_output "$expected_output2"

}
#WEIGHT=16
