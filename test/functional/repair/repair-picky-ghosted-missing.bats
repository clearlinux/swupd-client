#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/foo -d/usr/share/clear/bundles "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.test-bundle file-status /usr/foo .g..
	# since the files must have been installed by the -L option, remove /usr/foo
	sudo rm -f "$TARGETDIR"/usr/foo

}

@test "REP029: When repairing ghosted files are not added during --picky" {

	run sudo sh -c "$SWUPD repair --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		--picky removing extra files under .*/target-dir/usr
		REMOVING /usr/share/defaults/swupd/versionurl
		REMOVING /usr/share/defaults/swupd/contenturl
		Inspected 15 files
		  2 files found which should be deleted
		    2 of 2 files were deleted
		    0 of 2 files were not deleted
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# this should not exist at the end, despite being marked in the Manifest as
	# "ghosted". In this case ghosted files should be ignored.
	assert_file_not_exists "$TARGETDIR"/usr/foo

}
