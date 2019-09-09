#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/foo -d/usr/share/clear/bundles "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.test-bundle file-status /usr/foo .g..

}

@test "REP028: When repairing ghosted files are skipped during --picky" {

	run sudo sh -c "$SWUPD repair --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Download missing manifests ...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Removing extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/contenturl -> deleted
		Inspected 15 files
		  2 files found which should be deleted
		    2 of 2 files were deleted
		    0 of 2 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, despite being marked as "ghosted" in the
	# Manifest. With the ghosted file existing under /usr this test is to make
	# sure the ghosted files aren't removed during the --picky flow.
	assert_file_exists "$TARGETDIR"/usr/foo

}
