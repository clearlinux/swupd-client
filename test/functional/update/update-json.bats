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

	run sudo sh -c "$SWUPD update --json-output $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		\[
		\{ "type" : "start", "section" : "update" \},
		\{ "type" : "info", "msg" : "Update started. " \},
		\{ "type" : "progress", "currentStep" : 1, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "prepare_for_update" \},
		\{ "type" : "info", "msg" : "Preparing to update from 10 to 20 " \},
		\{ "type" : "progress", "currentStep" : 2, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "get_versions" \},
		\{ "type" : "progress", "currentStep" : 3, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "cleanup_download_dir" \},
		\{ "type" : "progress", "currentStep" : 4, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "load_manifests" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "run_preupdate_scripts" \},
		\{ "type" : "info", "msg" : "Downloading packs... " \},
		.*
		.*
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "apply_deltas" \},
		\{ "type" : "info", "msg" : "Statistics for going from version 10 to version 20:  " \},
		\{ "type" : "info", "msg" : "    changed bundles   : 1 " \},
		\{ "type" : "info", "msg" : "    new bundles       : 0 " \},
		\{ "type" : "info", "msg" : "    deleted bundles   : 0  " \},
		\{ "type" : "info", "msg" : "    changed files     : 2 " \},
		\{ "type" : "info", "msg" : "    new files         : 1 " \},
		\{ "type" : "info", "msg" : "    deleted files     : 0 " \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "create_update_list" \},
		\{ "type" : "info", "msg" : "Starting download of remaining update content. This may take a while... " \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 11, "stepCompletion" : 33, "stepDescription" : "download_fullfiles" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 11, "stepCompletion" : 66, "stepDescription" : "download_fullfiles" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" \},
		\{ "type" : "info", "msg" : "Finishing download of update content... " \},
		\{ "type" : "info", "msg" : "Staging file content " \},
		\{ "type" : "info", "msg" : "Applying update " \},
		\{ "type" : "progress", "currentStep" : 10, "totalSteps" : 11, "stepCompletion" : 33, "stepDescription" : "update_files" \},
		\{ "type" : "progress", "currentStep" : 10, "totalSteps" : 11, "stepCompletion" : 66, "stepDescription" : "update_files" \},
		\{ "type" : "progress", "currentStep" : 10, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "update_files" \},
		\{ "type" : "info", "msg" : "Update was applied. " \},
		\{ "type" : "info", "msg" : "Calling post-update helper scripts. " \},
		\{ "type" : "progress", "currentStep" : 11, "totalSteps" : 11, "stepCompletion" : 100, "stepDescription" : "run_postupdate_scripts" \},
		\{ "type" : "info", "msg" : "Update took 0.0 seconds, 0 MB transferred " \},
		\{ "type" : "info", "msg" : "Update successful. System updated from version 10 to version 20 " \},
		\{ "type" : "end", "section" : "update", "status" : 0 \}
		\]
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_in_output "{ \"type\" : \"progress\", \"currentStep\" : 6, \"totalSteps\" : 11, \"stepCompletion\" : 100, \"stepDescription\" : \"download_packs\" },"

}
