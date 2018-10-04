#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle-1 -f /test-file1 "$TEST_NAME"
	create_bundle -n test-bundle-2 -f /test-file2,/test-file3 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle -p "$TEST_NAME" test-bundle-1 --header-only
	add_dependency_to_manifest "$WEBDIR"/20/Manifest.test-bundle-1 test-bundle-2
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /test-file1
	update_bundle -i "$TEST_NAME" test-bundle-2 --update /test-file2

}

@test "update works correctly using an iterative manifest that has a new dependency" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
	# verify downloaded manifests
	# (test-bundle-2 was added in version 20 but it was not installed in the
	# target system, so its iterative manifest should not be used)
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-1.D.10
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-1.I.10
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-2
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/10/Manifest.test-bundle-1
	assert_file_not_exists "$STATEDIR"/10/Manifest.test-bundle-2
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-1
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-2.I.10
	assert_file_not_exists "$STATEDIR"/10/Manifest.os-core
	assert_file_not_exists "$STATEDIR"/20/Manifest.os-core

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting test-bundle-2 pack for version 20
		Extracting test-bundle-1 pack for version 20
		Couldn.t use delta file .* no .from. file to apply was found
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 1
		    deleted bundles   : 0
		    changed files     : 4
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		3 files were not in a pack
		Update successful. System updated from version 10 to version 20
	EOM
	)
	assert_regex_is_output "$expected_output"

}

