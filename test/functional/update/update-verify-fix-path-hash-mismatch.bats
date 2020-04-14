#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/foo/test-file "$TEST_NAME"
	# remove the installed foo directory
	sudo rm -rf "$TARGETDIR"/usr/foo
	# change permissions of the installed usr directory so the hash doesn't match
	sudo chmod g+w "$TARGETDIR"/usr
	# create an update
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --update /usr/foo/test-file

}

@test "UPD030: Update corrects a directory that has a hash mismatch" {

	# pre-test validations
	assert_dir_not_exists "$TARGETDIR"/usr/foo
	file_hash=$(sudo "$SWUPD" hashdump --quiet "$TARGETDIR"/usr)
	good_hash=$(get_hash_from_manifest "$WEBDIR"/100/Manifest.test-bundle /usr)
	assert_not_equal "$file_hash" "$good_hash"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
		Downloading packs for:
		 - test-bundle
		Finishing packs extraction...
		Warning: Couldn.t use delta file .*
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		1 files were not in a pack
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_dir_exists "$TARGETDIR"/usr/foo
	assert_file_exists "$TARGETDIR"/usr/foo/test-file

}
#WEIGHT=3
