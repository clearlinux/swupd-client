#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /file_1,/file_2 "$TEST_NAME"

}

@test "SRH027: Search for a file using machine readable output" {

	sudo sh -c "$SWUPD search-file --init $SWUPD_OPTS"

	# the --json-output flag can be used so all the output of the search
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD search-file --json-output $SWUPD_OPTS file_1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "search" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 3, "stepCompletion" : 100, "stepDescription" : "get_versions" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 3, "stepCompletion" : 50, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 3, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "info", "msg" : "Searching for 'file_1' " },
		{ "type" : "info", "msg" : " Bundle test-bundle " },
		{ "type" : "info", "msg" : "(0 MB to install)" },
		{ "type" : "info", "msg" : "	/file_1 " },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 3, "stepCompletion" : 100, "stepDescription" : "search_term" },
		{ "type" : "end", "section" : "search", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
