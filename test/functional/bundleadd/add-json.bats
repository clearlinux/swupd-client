#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	test_files="/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10,/file11"
	create_bundle -n test-bundle -f "$test_files" "$TEST_NAME"

}

@test "ADD049: Adding a bundle using machine readable output" {

	# the --json-output flag can be used so all the output of the bundle-add
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command
	# (for example installer so it can provide the user a status of the install process)

	run sudo sh -c "$SWUPD bundle-add --json-output $SWUPD_OPTS test-bundle"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "bundle-add" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "download_bundle_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "consolidate_files" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "check_disk_space_availability" },
		{ "type" : "info", "msg" : "Downloading packs... " },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "download_packs" },
		{ "type" : "info", "msg" : " Extracting test-bundle pack for version 10 " },
		{ "type" : "info", "msg" : "Starting download of remaining update content. This may take a while... " },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 8, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 16, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 25, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 33, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 50, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 66, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 75, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 83, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 91, "stepDescription" : "download_fullfiles" },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" },
		{ "type" : "info", "msg" : "Finishing download of update content... " },
		{ "type" : "info", "msg" : "Installing bundle(s) files... " },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 4, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 8, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 12, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 16, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 20, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 25, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 29, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 33, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 37, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 45, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 50, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 54, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 62, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 66, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 70, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 75, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 79, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 83, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 87, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 91, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 95, "stepDescription" : "installing_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "installing_files" },
		{ "type" : "info", "msg" : "Calling post-update helper scripts. " },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "run_scripts" },
		{ "type" : "info", "msg" : "Successfully installed 1 bundle " },
		{ "type" : "end", "section" : "bundle-add", "status" : 0 }
		]
	EOM
	)
	assert_is_output "$expected_output"

}
