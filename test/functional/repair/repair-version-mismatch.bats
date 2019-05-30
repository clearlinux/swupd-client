#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --add-dir /usr/bin
	set_current_version "$TEST_NAME" 20

}

@test "REP015: Repair enforces the use of the correct version" {

	run sudo sh -c "$SWUPD repair -m 10 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Error: Repairing to a different version requires --force or --picky
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
