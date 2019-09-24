#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /file_1 "$TEST_NAME"

}

@test "REM020: Removing a bundle using machine readable output" {

	# the --json-output flag can be used so all the output of the bundle-remove
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD bundle-remove --json-output $SWUPD_OPTS test-bundle"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "bundle-remove" },
		{ "type" : "info", "msg" : " Removing bundle: test-bundle " },
		{ "type" : "info", "msg" : " Deleting bundle files... " },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 1, "stepCompletion" : 50, "stepDescription" : "remove_files" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 1, "stepCompletion" : 100, "stepDescription" : "remove_files" },
		{ "type" : "info", "msg" : "Total deleted files: 2 " },
		{ "type" : "info", "msg" : " Successfully removed 1 bundle " },
		{ "type" : "end", "section" : "bundle-remove", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
