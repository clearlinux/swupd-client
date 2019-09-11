#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 20 10

}

@test "CHK009: Check for updates using machine readable output" {

	# the --json-output flag can be used so all the output of the check-update
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD check-update --json-outpu $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "check-update" },
		{ "type" : "info", "msg" : "Current OS version: 10" },
		{ "type" : "info", "msg" : "Latest server version: 20" },
		{ "type" : "info", "msg" : "There is a new OS version available: 20 " },
		{ "type" : "end", "section" : "check-update", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
