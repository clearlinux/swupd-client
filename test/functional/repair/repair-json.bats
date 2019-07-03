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

@test "REP020: Repair a system using machine readable output" {

	run sudo sh -c "$SWUPD repair --json-output --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output1=$(cat <<-EOM
		\[
		\{ "type" : "start", "section" : "verify" \},
		\{ "type" : "progress", "currentStep" : 1, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "get_versions" \},
		\{ "type" : "info", "msg" : "Diagnosing version 10 " \},
		\{ "type" : "progress", "currentStep" : 2, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "cleanup_download_dir" \},
		\{ "type" : "progress", "currentStep" : 3, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "load_manifests" \},
		\{ "type" : "progress", "currentStep" : 4, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "consolidate_files" \},
		\{ "type" : "info", "msg" : "Checking for corrupt files " \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 5, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 11, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 17, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 23, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 29, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 35, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 41, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 47, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 52, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 58, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 64, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 70, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 76, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 82, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 88, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 94, "stepDescription" : "check_files_hash" \},
		\{ "type" : "progress", "currentStep" : 5, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "check_files_hash" \},
		\{ "type" : "info", "msg" : "Starting download of remaining update content. This may take a while... " \},
	EOM
	)
	expected_output2=$(cat <<-EOM
		\{ "type" : "progress", "currentStep" : 6, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" \},
		\{ "type" : "info", "msg" : " Adding any missing files " \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 5, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 11, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : "  -> Missing file: .*/baz" \},
		\{ "type" : "info", "msg" : " -> fixed " \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 17, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 23, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 29, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : "  -> Missing file: .*/foo/test-file1" \},
		\{ "type" : "info", "msg" : " -> fixed " \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 35, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 41, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 47, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 52, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 58, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 64, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 70, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 76, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 82, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 88, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 94, "stepDescription" : "add_missing_files" \},
		\{ "type" : "progress", "currentStep" : 7, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "add_missing_files" \},
		\{ "type" : "info", "msg" : " Repairing corrupt files " \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 5, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 11, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 17, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 23, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 29, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 35, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 41, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 47, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 52, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 58, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 64, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 70, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 76, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 82, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 88, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 94, "stepDescription" : "fix_files" \},
		\{ "type" : "progress", "currentStep" : 8, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "fix_files" \},
		\{ "type" : "info", "msg" : " Removing extraneous files " \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 5, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 11, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 17, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 23, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 29, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 35, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 41, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 47, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 52, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 58, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 64, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 70, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 76, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 82, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 88, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 94, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "progress", "currentStep" : 9, "totalSteps" : 9, "stepCompletion" : 100, "stepDescription" : "remove_extraneous_files" \},
		\{ "type" : "info", "msg" : " --picky removing extra files under .*/usr " \},
		\{ "type" : "info", "msg" : "REMOVING /usr/share/defaults/swupd/versionurl " \},
		\{ "type" : "info", "msg" : "REMOVING /usr/share/defaults/swupd/contenturl " \},
		\{ "type" : "info", "msg" : "REMOVING /usr/extraneous-file " \},
		\{ "type" : "info", "msg" : " Inspected 20 files " \},
		\{ "type" : "info", "msg" : "  2 files were missing " \},
		\{ "type" : "info", "msg" : "    2 of 2 missing files were replaced " \},
		\{ "type" : "info", "msg" : "    0 of 2 missing files were not replaced " \},
		\{ "type" : "info", "msg" : "  3 files found which should be deleted " \},
		\{ "type" : "info", "msg" : "    3 of 3 files were deleted " \},
		\{ "type" : "info", "msg" : "    0 of 3 files were not deleted " \},
		\{ "type" : "info", "msg" : "Calling post-update helper scripts " \},
		\{ "type" : "warning", "msg" : "helper script \\(.*/usr/bin/clr-boot-manager\\) not found, it will be skipped " \},
		\{ "type" : "info", "msg" : " Repair successful " \},
		\{ "type" : "end", "section" : "verify", "status" : 0 \}
		\]
	EOM
	)
	assert_regex_in_output "$expected_output1"
	assert_regex_in_output "$expected_output2"

}
