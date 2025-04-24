#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /file1,/file2,/file3 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --update /file1
	update_bundle "$TEST_NAME" test-bundle --update /file2
	update_bundle "$TEST_NAME" test-bundle --add /file4

}

@test "UPD056: Updating a system using machine readable output" {

	# the --json-output flag can be used so all the output of the update
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD update --json-output $SWUPD_OPTS_PROGRESS"

	assert_status_is 0
	expected_output1=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "update" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "load_manifests" },
		{ "type" : "info", "msg" : "Update started" },
		{ "type" : "info", "msg" : "Preparing to update from 10 to 20 (in format staging)" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "run_preupdate_scripts" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "run_preupdate_scripts" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 10, "stepCompletion" : 0, "stepDescription" : "download_packs" },
		{ "type" : "info", "msg" : "Downloading packs for:" },
		{ "type" : "info", "msg" : " - test-bundle" },
		{ "type" : "info", "msg" : "Finishing packs extraction..." },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "download_packs" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "extract_packs" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "extract_packs" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "prepare_for_update" },
		{ "type" : "info", "msg" : "Statistics for going from version 10 to version 20:" },
		{ "type" : "info", "msg" : "    changed bundles   : 1" },
		{ "type" : "info", "msg" : "    new bundles       : 0" },
		{ "type" : "info", "msg" : "    deleted bundles   : 0" },
		{ "type" : "info", "msg" : "    changed files     : 2" },
		{ "type" : "info", "msg" : "    new files         : 1" },
		{ "type" : "info", "msg" : "    deleted files     : 0" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "prepare_for_update" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 10, "stepCompletion" : 0, "stepDescription" : "validate_fullfiles" },
		{ "type" : "info", "msg" : "Validate downloaded files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 10, "stepCompletion" : 33, "stepDescription" : "validate_fullfiles" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 10, "stepCompletion" : 66, "stepDescription" : "validate_fullfiles" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "validate_fullfiles" },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 10, "stepCompletion" : 0, "stepDescription" : "download_fullfiles" },
		{ "type" : "info", "msg" : "No extra files need to be downloaded" },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "extract_fullfiles" },
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "extract_fullfiles" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 0, "stepDescription" : "update_files" },
		{ "type" : "info", "msg" : "Installing files..." },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 16, "stepDescription" : "update_files" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 33, "stepDescription" : "update_files" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 50, "stepDescription" : "update_files" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 66, "stepDescription" : "update_files" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 83, "stepDescription" : "update_files" },
		{ "type" : "progress", "currentStep" : 9, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "update_files" },
		{ "type" : "info", "msg" : "Update was applied" },
		{ "type" : "progress", "currentStep" : 10, "totalSteps" : 10, "stepCompletion" : -1, "stepDescription" : "run_postupdate_scripts" },
		{ "type" : "info", "msg" : "Calling post-update helper scripts" },
	EOM
	)
	expected_output2=$(cat <<-EOM
		\{ "type" : "info", "msg" : "Update took ... seconds, 0 MB transferred" \},
		\{ "type" : "info", "msg" : "Update successful - System updated from version 10 to version 20" \},
		\{ "type" : "progress", "currentStep" : 10, "totalSteps" : 10, "stepCompletion" : 100, "stepDescription" : "run_postupdate_scripts" \},
		\{ "type" : "end", "section" : "update", "status" : 0 \}
		\]
	EOM
	)
	assert_in_output "$expected_output1"
	assert_regex_in_output "$expected_output2"

}
#WEIGHT=5
