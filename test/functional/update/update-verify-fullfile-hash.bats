#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --update /core
	file_hash=$(get_hash_from_manifest "$WEB_DIR"/100/Manifest.os-core /core)
	# remove the /usr/bin/core file from the system
	sudo rm -f "$TARGET_DIR"/core
	# remove packs so fullfiles are used
	sudo rm -rf "$WEB_DIR"/100/pack-*
	# modify the fullfile so it is corrupt
	write_to_protected_file -a "$WEB_DIR"/100/files/"$file_hash" "some extra stuff to break the hash"
	sudo rm "$WEB_DIR"/100/files/"$file_hash".tar
	create_tar "$WEB_DIR"/100/files/"$file_hash"

}

@test "UPD028: Try updating a system having a corrupt fullfile in the server" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 1
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100 \(in format staging\)
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Error: File content hash mismatch for .* \\(bad server data\\?\\)
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/core

}
#WEIGHT=2
