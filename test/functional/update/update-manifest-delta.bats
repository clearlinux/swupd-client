#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1,/foo/file_2 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /file_b2 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_3
	create_version -p "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" test-bundle2 --add /bar/file_b3

	create_delta_manifest test-bundle1 20 10
	create_delta_manifest test-bundle2 30 10

	sudo rm "$WEBDIR/20/Manifest.test-bundle1"
	sudo rm "$WEBDIR/20/Manifest.test-bundle1.tar"
	sudo rm "$WEBDIR/30/Manifest.test-bundle2"
	sudo rm "$WEBDIR/30/Manifest.test-bundle2.tar"

}

@test "UPD053: Updating a system using manifest-deltas" {

	# updating successfully from a server without manifests for 20, but
	# with correct delta manifests

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -V 20"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 2
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD054: Updating a system using manifest-deltas jumping one release" {

	# Updating successfully from a server without Manifests for 20 and 30, but
	# with correct delta manifests

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -V 30"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 30
		Downloading packs for:
		 - test-bundle1
		 - test-bundle2
		Finishing packs extraction...
		Statistics for going from version 10 to version 30:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 3
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD063: Error when manifest delta is invalid" {

	# Updating fails when there's no Manifests for 20 and 30 and delta
	# Manifests are valid but generating and incorect Manifest.

	sudo mv "$WEBDIR"/20/Manifest-test-bundle1-delta-from-10 "$WEBDIR"/30/Manifest-test-bundle2-delta-from-10

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -V 30"

	assert_status_is "$SWUPD_RECURSE_MANIFEST"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 30
		Error: Failed to retrieve 30 test-bundle2 manifest
		Error: Unable to download manifest test-bundle2 version 30, exiting now
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=14
