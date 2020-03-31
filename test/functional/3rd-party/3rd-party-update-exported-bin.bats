#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create an initial environment
	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1,/usr/bin/binary_1,/bin/binary_2,/bin/binary_3 -u repo1 "$TEST_NAME"
	create_bundle       -n test-bundle2 -f /bar/file_2,/bin/binary_4                                 -u repo1 "$TEST_NAME"
	create_bundle       -n test-bundle3 -f /bin/binary_5                                             -u repo1 "$TEST_NAME"

	# create an update that adds, removes and updates binaries
	create_version -p "$TEST_NAME" 20 10 staging repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1                 repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /bin/binary_2               repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --add    /usr/local/bin/binary_6     repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /usr/bin/binary_1           repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --rename /bin/binary_3:/bin/binary_7 repo1
	add_dependency_to_manifest "$TPWEBDIR"/20/Manifest.test-bundle1 test-bundle3

	# let's add a line to the script for binary_2 so we can tell if it was
	# re-generated after the update
	write_to_protected_file -a "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2 "TEST_STRING"

}

@test "TPR059: Update a 3rd-party bundle that has exported binaries" {

	# pre-test checks
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3

	# During an update, a bundle that contains binaries should have
	# their scripts updated too

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________
		 3rd-Party Repo: repo1
		_______________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle3
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 6
		    deleted files     : 2
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_1
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_3
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_4
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_5
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_6
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_7
	sudo grep -vq "TEST_STRING" "$TARGETDIR"/"$THIRD_PARTY_BIN_DIR"/binary_2 || exit 1

}
#WEIGHT=9
