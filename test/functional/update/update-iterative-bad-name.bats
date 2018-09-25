#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle-1 -f /test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle-2 -f /test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle-3 -f /test-file3 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /test-file1
	update_bundle -i "$TEST_NAME" test-bundle-2 --update /test-file2
	update_bundle -i "$TEST_NAME" test-bundle-3 --update /test-file3
	# modify the iterative manifests so they have a bad name
	sudo mv "$WEBDIR"/20/Manifest.test-bundle-1.I.10 "$WEBDIR"/20/Manifest.test-bundle-1.F.123
	update_manifest -p "$WEBDIR"/20/Manifest.MoM file-name test-bundle-1.I.10 test-bundle-1.F.123
	sudo mv "$WEBDIR"/20/Manifest.test-bundle-2.I.10 "$WEBDIR"/20/Manifest.test-bundle-2.I.1A0
	update_manifest -p "$WEBDIR"/20/Manifest.MoM file-name test-bundle-2.I.10 test-bundle-2.I.1A0
	sudo mv "$WEBDIR"/20/Manifest.test-bundle-3.I.10 "$WEBDIR"/20/Manifest.test-bundle-3I10
	update_manifest "$WEBDIR"/20/Manifest.MoM file-name test-bundle-3.I.10 test-bundle-3I10

}

@test "update ignores iterative manifests with a bad name" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
	# verify downloaded manifests
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-2
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-3
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-2
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-3
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-1.F.123
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-2.I.1A0
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-3I10
	assert_file_not_exists "$STATEDIR"/10/Manifest.os-core
	assert_file_not_exists "$STATEDIR"/20/Manifest.os-core

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting test-bundle-3 pack for version 20
		Extracting test-bundle-2 pack for version 20
		Extracting test-bundle-1 pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 3
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
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
