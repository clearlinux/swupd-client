#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2
	update_bundle "$TEST_NAME" os-core --add-dir /usr/bin

}

@test "REP018: Repair can be forced to continue when there is a format mismatch" {

	# the -x option bypasses the fatal error
	run sudo sh -c "$SWUPD repair --format=1 --version=40 --force $SWUPD_OPTS_NO_FMT"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 40
		Warning: The --force option is specified; ignoring format mismatch for diagnose
		Warning: The --force option is specified; ignoring version mismatch for repair
		Download missing manifests
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $TEST_DIRNAME/testfs/target-dir/usr/bin -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $TEST_DIRNAME/testfs/target-dir/usr/lib/os-release -> fixed
		 -> Hash mismatch for file: $TEST_DIRNAME/testfs/target-dir/usr/share/defaults/swupd/format -> fixed
		Removing extraneous files
		Inspected 12 files
		  1 file was missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		  2 files did not match
		    2 of 2 files were repaired
		    0 of 2 files were not repaired
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
