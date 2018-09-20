#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -L -n bundle1 -v 10 -f /file1 "$TEST_NAME"
	create_bundle -L -n bundle2 -v 10 -f /file2 "$TEST_NAME"
	create_bundle -L -n bundle3 -v 10 -f /file3 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10 staging
	update_bundle -i "$TEST_NAME" bundle1 --update /file1
	update_bundle "$TEST_NAME" bundle2 --update /file2
}

@test "update with delta manifest and full manifest" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"
	assert_status_is 0

	# Verify target files
	assert_file_exists "$TARGETDIR"/file1
	assert_file_exists "$TARGETDIR"/file2
	assert_file_exists "$TARGETDIR"/file3

	# Verify downloaded manifests
	assert_file_exists "$STATEDIR"/20/Manifest.bundle1.D.10
	assert_file_exists "$STATEDIR"/20/Manifest.bundle1.I.10
	assert_file_exists "$STATEDIR"/10/Manifest.bundle2
	assert_file_exists "$STATEDIR"/20/Manifest.bundle2

	# Verify unnecessary manifest not downloaded
	assert_file_not_exists "$STATEDIR"/10/Manifest.bundle1
	assert_file_not_exists "$STATEDIR"/10/Manifest.bundle3
	assert_file_not_exists "$STATEDIR"/20/Manifest.bundle3

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting bundle2 pack for version 20
		Extracting bundle1 pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
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
