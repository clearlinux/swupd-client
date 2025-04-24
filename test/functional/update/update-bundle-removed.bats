#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version -r "$TEST_NAME" 20 10
	remove_from_manifest "$WEB_DIR"/20/Manifest.MoM test-bundle1

}

@test "UPD008: Updating a system where a bundle was removed in the newer version" {

	# If a bundle happens to be removed from the content server (or mix) it means the
	# bundle won't be in the MoM anymore, this now indicates a bundle delete.

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 1
		    changed files     : 2
		    new files         : 0
		    deleted files     : 2
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
	# bundle should not be removed
	assert_file_not_exists "$TARGET_DIR"/file_1
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1

}
#WEIGHT=5
