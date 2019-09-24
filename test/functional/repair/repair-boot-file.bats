#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	# change the content of testfile so the hash doesn't match
	write_to_protected_file -a "$TARGETDIR"/usr/lib/kernel/testfile "some new content"

}

@test "REP003: Repair a system that has a corrupt boot file" {

	# <If necessary add a detailed explanation of the test here>

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $PATH_PREFIX/usr/lib/kernel/testfile -> fixed
		Removing extraneous files
		Inspected 7 files
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		Calling post-update helper scripts
		Warning: helper script ($PATH_PREFIX//usr/bin/clr-boot-manager) not found, it will be skipped
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
