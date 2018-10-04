#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle-1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle-2 -f /foo/test-file2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /foo/test-file1
	create_version "$TEST_NAME" 30 20
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /foo/test-file1
	update_bundle -i "$TEST_NAME" test-bundle-2 --update /foo/test-file2

}

@test "update works correctly using both full and iterative manifests in the same operation" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/foo/test-file2
	# verify downloaded manifests
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/30/Manifest.test-bundle-2.D.10
	assert_file_exists "$STATEDIR"/30/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/30/Manifest.test-bundle-2.I.10
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/30/Manifest.test-bundle-1.I.10
	assert_file_not_exists "$STATEDIR"/30/Manifest.test-bundle-1.I.20
	assert_file_not_exists "$STATEDIR"/30/Manifest.test-bundle-2
	assert_file_not_exists "$STATEDIR"/10/Manifest.test-bundle-2
	assert_file_not_exists "$STATEDIR"/10/Manifest.os-core
	assert_file_not_exists "$STATEDIR"/30/Manifest.os-core

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 30
		Downloading packs...
		Extracting test-bundle-2 pack for version 30
		Statistics for going from version 10 to version 30:
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
		1 files were not in a pack
		Update successful. System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

