#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /file_2

	# create a mirror at this point
	create_mirror "$TEST_NAME"

	# the following update won't be part of the mirror cause it
	# was done after the mirror was created
	create_version "$TEST_NAME" 1000 20
	update_bundle "$TEST_NAME" test-bundle1 --add /file_3

}

@test "UPD041: Updating a system using a stale mirror" {

	# When doing an update with a mirror set up, swupd will make sure
	# the mirror is not too far behind the upstream content server, if
	# it is, it unsets the mirror so the update uses the upstream server

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Checking mirror status
		Warning: the mirror version (20) is too far behind upstream version (1000)
		Removing mirror configuration
		Preparing to update from 10 to 1000
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 1000:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 2
		    deleted files     : 0
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 1000
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/file_2
	assert_file_exists "$TARGETDIR"/file_3

}
