#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"
	# set up state directory with bad hash file and pack hint
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle /usr/bin/test-file)
	sudo sh -c "echo \"test file MODIFIED\" > $STATEDIR/staged/$file_hash"
	sudo touch "$STATEDIR"/pack-test-bundle-from-0-to-10.tar

}

@test "ADD017: Try adding a bundle with a bad hash in the state directory" {

	# since one of the files needed to install the bundle is already in the state/staged
	# directory, in theory this one should be used instead of downloading it again...
	# however since the hash of this file is wrong it should be deleted and re-downloaded
	hash_before=$(sudo "$SWUPD" hashdump "$STATEDIR"/staged/"$file_hash")

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 0
	hash_after=$(sudo "$SWUPD" hashdump "$STATEDIR"/staged/"$file_hash")
	assert_file_exists "$TARGETDIR"/usr/bin/test-file
	assert_not_equal "$hash_before" "$hash_after"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=2
