#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	test_setup_gen

}

@test "REP017: Repair enforces the use of the correct format" {

	run sudo sh -c "$SWUPD repair --format=1 --version=40 $SWUPD_OPTS_NO_FMT"

	assert_status_is "$SWUPD_COULDNT_LOAD_MANIFEST"
	expected_output=$(cat <<-EOM
		Diagnosing version 40
		Error: Mismatching formats detected when diagnosing 40 (expected: 1; actual: 2)
		Latest supported version to diagnose: 20
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=7
