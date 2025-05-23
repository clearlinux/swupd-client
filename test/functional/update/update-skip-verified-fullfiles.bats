#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	filehash=$(get_hash_from_manifest "$WEB_DIR"/10/Manifest.test-bundle /foo)
	update_bundle "$TEST_NAME" test-bundle --update /foo
	# restore the hash for /foo in the manifest to the original one so it no longer matches
	manifest="$WEB_DIR"/100/Manifest.test-bundle
	update_manifest "$manifest" file-hash /foo "$filehash"
	# to make sure they are not used, remove packs and fullfiles
	sudo rm "$WEB_DIR"/100/pack-test-bundle-from-0.tar
	sudo rm "$WEB_DIR"/100/pack-test-bundle-from-10.tar
	sudo rm "$WEB_DIR"/100/delta/*
	sudo rm -rf "$WEB_DIR"/100/files/*

}

@test "UPD027: Update skips fullfile download when the hash for the file verifies correctly" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100 (in format staging)
		Downloading packs for:
		 - test-bundle
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/foo

}
#WEIGHT=3
