#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle-1 -f /test-file1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	create_version "$TEST_NAME" 30 20
	create_version "$TEST_NAME" 40 30
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /test-file1
	set_current_version "$TEST_NAME" 20

}

@test "update works correctly using iterative manifests when the bundle hadn't been updated in many versions" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/test-file1
	# verify downloaded manifests
	assert_file_exists "$STATEDIR"/40/Manifest.test-bundle-1.I.10
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/40/Manifest.test-bundle-1

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 20 to 40
		Downloading packs...
		Extracting test-bundle-1 pack for version 40
		Statistics for going from version 20 to version 40:
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
		Update successful. System updated from version 20 to version 40
	EOM
	)
	assert_is_output "$expected_output"

}

