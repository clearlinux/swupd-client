#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo,/bar,/bat/baz "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle -i "$TEST_NAME" test-bundle --update /foo
	sudo rm -rf "$WEBDIR"/20/Manifest.test-bundle.I.10
	sudo rm -rf "$WEBDIR"/20/Manifest.test-bundle.I.10.tar

}

@test "update falls back to full manifests when the iterative fails to download" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_KEEPCACHE"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/foo
	assert_file_exists "$TARGETDIR"/bar
	assert_file_exists "$TARGETDIR"/bat/baz

	# since the iterative manifest was deleted the full manifest should
	# have been downloaded
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle.D.10
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/10/Manifest.test-bundle
	assert_file_not_exists "$STATEDIR"/10/Manifest.os-core
	assert_file_not_exists "$STATEDIR"/20/Manifest.os-core
	# verify the iterative manifest is in the MoM
	iterative_manifest="$(get_hash_from_manifest "$WEBDIR"/20/Manifest.MoM test-bundle.I.10)"
	assert_true "$iterative_manifest" "The iterative manifest was not found in the MoM"

	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Downloading packs...
		Extracting test-bundle pack for version 20
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
