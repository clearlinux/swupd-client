#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /file_1 "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

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
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 2, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 2, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 2, "stepCompletion" : -1, "stepDescription" : "list_bundles" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 2, "stepCompletion" : 100, "stepDescription" : "list_bundles" },
		{ "type" : "end", "section" : "bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=2
