#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1,/bar/test-file2,/baz "$TEST_NAME"
	create_bundle -n test-bundle2 -f /test-file3,/test-file4 "$TEST_NAME"
    sudo rm -f "$TARGETDIR"/foo/test-file1
    sudo rm -f "$TARGETDIR"/baz
    sudo touch "$TARGETDIR"/usr/extraneous-file

}

@test "DIA010: Diagnose a system using machine readable output" {

	run sudo sh -c "$SWUPD diagnose --json-output $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		\[
		\{ "type" : "start", "section" : "verify" \},
		\{ "type" : "progress", "currentStep" : 1, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "get_versions" \},
		\{ "type" : "info", "msg" : "Diagnosing version 10 " \},
		\{ "type" : "progress", "currentStep" : 2, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "cleanup_download_dir" \},
		\{ "type" : "info", "msg" : "Downloading missing manifests... " \},
		\{ "type" : "progress", "currentStep" : 3, "totalSteps" : 7, "stepCompletion" : -1, "stepDescription" : "load_manifests" \},
		\{ "type" : "progress", "currentStep" : 3, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "load_manifests" \},
		\{ "type" : "progress", "currentStep" : 4, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "consolidate_files" \},
		\{ "type" : "info", "msg" : " Checking for missing files " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 5, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 11, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : " -> Missing file: .*/baz " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 17, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 23, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 29, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : " -> Missing file: .*/foo/test-file1 " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 35, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 47, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 52, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 64, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 70, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 76, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 82, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 88, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 94, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : " Checking for corrupt files " \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 5, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 11, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 17, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 23, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 29, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 35, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 47, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 52, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 64, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 70, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 76, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 82, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 88, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 94, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "fix_files" \},
		\{ "type" : "info", "msg" : " Checking for extraneous files " \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 5, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 11, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 17, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 23, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 29, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 35, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 47, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 52, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 64, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 70, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 76, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 82, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 88, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 94, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "info", "msg" : " Inspected 17 files " \},
		\{ "type" : "info", "msg" : "  2 files were missing " \},
		\{ "type" : "info", "msg" : " Use 'swupd repair' to correct the problems in the system " \},
		\{ "type" : "info", "msg" : " Diagnose successful " \},
		\{ "type" : "end", "section" : "verify", "status" : 1 \}
		\]
	EOM
	)
	assert_regex_in_output "$expected_output"

}
