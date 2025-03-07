#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle       -n test-bundle1 -f /file1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /file2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle3 -f /file3 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /file4 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file5 "$TEST_NAME"
	create_bundle -L    -n test-bundle6 -f /file6 "$TEST_NAME"
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle3 test-bundle4

	create_version -r "$TEST_NAME" 20 10
	remove_from_manifest "$WEB_DIR"/20/Manifest.MoM test-bundle3

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST027: List orphaned bundles" {

	# list bundles that are orphans

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle5
		 - test-bundle6
		Total: 2
		Use "swupd bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
		Use "swupd bundle-remove --orphans" to delete all orphaned bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "LST028: List orphaned bundles with status" {

	# list bundles that are orphans

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle5 (installed)
		 - test-bundle6 (installed)
		Total: 2
		Use "swupd bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
		Use "swupd bundle-remove --orphans" to delete all orphaned bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "LST030: Update deleting a bundle shows new orphans" {

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
	assert_file_not_exists "$TARGET_DIR"/file_3
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle3

        run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle4
		 - test-bundle5
		 - test-bundle6
		Total: 3
		Use "swupd bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
		Use "swupd bundle-remove --orphans" to delete all orphaned bundles
	EOM
	)
	assert_is_output "$expected_output"

}
