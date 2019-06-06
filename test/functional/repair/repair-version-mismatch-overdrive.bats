#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -d /usr/bin "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --header-only
	set_current_version "$TEST_NAME" 20
	# remove /usr/bin so it is missing in the target system
	sudo rm -rf "$TARGETDIR"/usr/bin

}

@test "REP016: Repair can be forced to continue when there is a version mismatch" {

	run sudo sh -c "$SWUPD repair -m 10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: the force or picky option is specified; ignoring version mismatch for repair
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		.* Missing file: .+/target-dir/usr/bin -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 3 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts.
		Repair successful
	EOM
	)
	assert_regex_is_output "$expected_output"

}
