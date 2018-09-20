#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -L -n "bundle1" -v 10 -f /file1 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10 staging
	update_bundle -i "$TEST_NAME" bundle1 --update /file1

	# Overwrite delta manifest with invalid file
	write_to_protected_file "$WEBDIR"/20/20/Manifest.bundle1.D.10 1
	sudo tar -C "$TEST_DIRNAME"/web-dir/20 -uf "$TEST_DIRNAME"/web-dir/20/pack-bundle1-from-10.tar 20/Manifest.bundle1.D.10
}

@test "update with invalid delta manifest and fallback" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"
	assert_status_is 0

	# Verify target file
	assert_file_exists "$TARGETDIR"/file1

	# Verify delta manifest and fallback downloaded
	assert_file_exists "$STATEDIR"/10/Manifest.bundle1
	assert_file_exists "$STATEDIR"/20/Manifest.bundle1.D.10

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting bundle1 pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
}
