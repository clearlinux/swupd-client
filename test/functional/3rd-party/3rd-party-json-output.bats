#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -n test-bundle2 -f /foo/file_2 -u repo2 "$TEST_NAME"

}

@test "TPR087: Using the --json-output flag with functions that run in a single repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1 --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-add" },
		{ "type" : "info", "msg" : "Searching for bundle test-bundle1 in the 3rd-party repositories..." },
		{ "type" : "info", "msg" : "Bundle test-bundle1 found in 3rd-party repository repo1" },
		{ "type" : "info", "msg" : " Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons" },
		{ "type" : "info", "msg" : "Loading required manifests..." },
		{ "type" : "info", "msg" : " Validating 3rd-party bundle binaries..." },
		{ "type" : "info", "msg" : "No packs need to be downloaded" },
		{ "type" : "info", "msg" : "Validate downloaded files" },
		{ "type" : "info", "msg" : "Starting download of remaining update content. This may take a while..." },
		{ "type" : "info", "msg" : " Validating 3rd-party bundle file permissions..." },
		{ "type" : "info", "msg" : "Installing files..." },
		{ "type" : "warning", "msg" : "post-update helper scripts skipped due to --no-scripts argument" },
		{ "type" : "info", "msg" : " Exporting 3rd-party bundle binaries..." },
		{ "type" : "info", "msg" : "Successfully installed 1 bundle" },
		{ "type" : "end", "section" : "3rd-party-bundle-add", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR088: Using the --json-output flag with functions that run in a single repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS_PROGRESS test-bundle1 --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "3rd-party-bundle-add" },
		{ "type" : "info", "msg" : "Searching for bundle test-bundle1 in the 3rd-party repositories..." },
		{ "type" : "info", "msg" : "Bundle test-bundle1 found in 3rd-party repository repo1" },
		{ "type" : "info", "msg" : " Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 11, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "info", "msg" : "Loading required manifests..." },
		{ "type" : "info", "msg" : " Validating 3rd-party bundle binaries..." },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "validate_binaries" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "validate_binaries" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "download_packs" },
		{ "type" : "info", "msg" : "No packs need to be downloaded" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "download_packs" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 11, "stepCompletion" : -1, "stepDescription" : "extract_packs" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "extract_packs" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "validate_fullfiles" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "validate_fullfiles" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "download_fullfiles" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 11, "stepCompletion" : -1, "stepDescription" : "extract_fullfiles" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "extract_fullfiles" },
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "validate_file_permissions" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "validate_file_permissions" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "install_files" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 10, "totalSteps" : 11, "stepCompletion" : -1, "stepDescription" : "run_postupdate_scripts" },
		{ "type" : "warning", "msg" : "post-update helper scripts skipped due to --no-scripts argument" },
		{ "type" : "info", "msg" : " Exporting 3rd-party bundle binaries..." },
		{ "type" : "progress", "currentStep" : 10, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "run_postupdate_scripts" },
		{ "type" : "progress", "currentStep" : 11, "totalSteps" : 11, "stepCompletion" : 0, "stepDescription" : "export_binaries" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "info", "msg" : "Successfully installed 1 bundle" },
		{ "type" : "end", "section" : "3rd-party-bundle-add", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}
