#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

}

@test "MIR006: Set a mirror using machine readable output" {

	# the --json-output flag can be used so all the output of the mirror
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD mirror --json-output -s http://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "mirror" },
		{ "type" : "info", "msg" : "Set upstream mirror to http://example.com/swupd-file " },
		{ "type" : "info", "msg" : "Installed version: 10 " },
		{ "type" : "info", "msg" : "Version URL:       http://example.com/swupd-file " },
		{ "type" : "info", "msg" : "Content URL:       http://example.com/swupd-file " },
		{ "type" : "end", "section" : "mirror", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
