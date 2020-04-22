#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

	# create a couple of 3rd-party repos with bundles
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle -L -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo2

}

@test "TPR089: Using the --json-output flag with functions that run in multiple repos specifying a repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --json-output --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-list" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle1" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "end", "section" : "3rd-party-bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR090: Using the --json-output flag to show progress with functions that run in multiple repos specifying a repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS_PROGRESS --json-output --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-list" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 2, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 2, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 2, "stepCompletion" : -1, "stepDescription" : "list_bundles" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle1" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 2, "stepCompletion" : 100, "stepDescription" : "list_bundles" },
		{ "type" : "end", "section" : "3rd-party-bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR091: Using the --json-output flag with functions that run in multiple repos" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-list" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : " 3rd-Party Repository: repo1" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle1" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : " 3rd-Party Repository: repo2" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " Total: 1" },
		{ "type" : "end", "section" : "3rd-party-bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR092: Using the --json-output flag to show progress with functions that run in multiple repos" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS_PROGRESS --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-list" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : " 3rd-Party Repository: repo1" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 4, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 4, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 4, "stepCompletion" : -1, "stepDescription" : "list_bundles" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "test-bundle1" },
		{ "type" : "info", "msg" : " Total: 2" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : " 3rd-Party Repository: repo2" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 4, "stepCompletion" : 100, "stepDescription" : "list_bundles" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 4, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 4, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 4, "stepCompletion" : -1, "stepDescription" : "list_bundles" },
		{ "type" : "info", "msg" : "Installed bundles:" },
		{ "type" : "info", "msg" : " -" },
		{ "type" : "info", "msg" : "os-core" },
		{ "type" : "info", "msg" : " Total: 1" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 4, "stepCompletion" : 100, "stepDescription" : "list_bundles" },
		{ "type" : "end", "section" : "3rd-party-bundle-list", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}
