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

@test "VER010: Verify a system using machine readable output" {

	run sudo sh -c "$SWUPD verify --json-output $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		\[
		\{ "type" : "start", "section" : "verify" \},
		\{ "type" : "progress", "currentStep" : 1, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "get_versions" \},
		\{ "type" : "info", "msg" : "Verifying version 10 " \},
		\{ "type" : "progress", "currentStep" : 2, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "cleanup_download_dir" \},
		\{ "type" : "progress", "currentStep" : 3, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "load_manifests" \},
		\{ "type" : "progress", "currentStep" : 4, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "consolidate_files" \},
		\{ "type" : "info", "msg" : "Verifying files " \},
		\{ "type" : "info", "msg" : " Missing file: .*/baz " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 6, "stepCompletion" : 17, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : " Missing file: .*/foo/test-file1 " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 6, "stepCompletion" : 35, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 5, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 11, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 17, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 23, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 29, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 35, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 41, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 47, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 52, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 58, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 64, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 70, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 76, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 82, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 88, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 94, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 6, "stepCompletion" : 100, "stepDescription" : "fix_files" \},
		\{ "type" : "info", "msg" : "Inspected 17 files " \},
		\{ "type" : "info", "msg" : "  2 files were missing " \},
		\{ "type" : "info", "msg" : "Verify successful " \},
		\{ "type" : "end", "section" : "verify", "status" : 1 \}
		\]
	EOM
	)
	assert_regex_is_output "$expected_output"

}
