#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

}

@test "REP016: Repair can be forced to continue when there is a version mismatch" {

	run sudo sh -c "$SWUPD repair -m 10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: The --force option is specified; ignoring version mismatch for repair
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/usr/bin -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 3 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=2
