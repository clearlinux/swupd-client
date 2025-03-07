#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -f /file1 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /file2 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /file3 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /file4 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file5 "$TEST_NAME"
	sudo touch "$state_path"/bundles/os-core
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle4 test-bundle5

	create_version -r "$TEST_NAME" 20 10
	remove_from_manifest "$WEB_DIR"/20/Manifest.MoM test-bundle1

}

@test "REM030: Remove orphan bundles" {

	# User can remove all bundles that are no longer required by
	# tracked bundles

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle5
		 - test-bundle4
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 3 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM031: Try removing orphan bundles along with other bundles" {

	# User cannot provide other bundles to be removed when using the
	# --orphans flag since this could potentially change the list of
	# orphans

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 --orphans"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: you cannot specify bundles to remove when using --orphans
	EOM
	)
	assert_in_output "$expected_output"

}

@test "REM033: Update deleting bundle adds new orphans that are removed" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 1
		    changed files     : 0
		    new files         : 7
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
	# bundle should not exist
	assert_file_not_exists "$TARGET_DIR"/file_1
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle5
		 - test-bundle4
		 - test-bundle3
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 8
		Successfully removed 4 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
