#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version -r "$TEST_NAME" 20 10
	remove_from_manifest "$WEBDIR"/20/Manifest.MoM test-bundle1

}

@test "UPD008: Updating a system where a bundle was removed in the newer version" {

	# If a bundle happens to be removed from the content server (or mix) it means the
	# bundle won't be in the MoM anymore, so the bundle in the system will look like an
	# invalid bundle. If this happens, the user should be informed, and the update should
	# continue.

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 20
		Warning: Bundle "test-bundle1" is invalid, skipping it...
		WARNING: One or more installed bundles are no longer available at version 20.
		Downloading packs...
		Extracting os-core pack for version 20
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
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
	# bundle should not be removed
	assert_file_exists "$TARGETDIR"/file_1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1

}
