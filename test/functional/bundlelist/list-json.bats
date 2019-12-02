#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /file_1 "$TEST_NAME"

}

@test "LST019: Listing bundles using machine readable output" {

	# the --json-output flag can be used so all the output of the bundle-list
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD bundle-list --json-output $SWUPD_OPTS_PROGRESS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "bundle-list" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "end", "section" : "bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_is_output --identical "$expected_output"

}
