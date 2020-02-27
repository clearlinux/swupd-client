#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

}

@test "REP014: Repair a system with extra files that are unsafe to delete" {

	# when running repair and there are extra files, but the extra
	# files are found not safe to be deleted, they should be skipped and
	# users should not be notified about these

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Inspected 9 files
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
