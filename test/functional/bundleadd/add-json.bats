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
	expected_output1=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "bundle-add" },
		{ "type" : "info", "msg" : "Loading required manifests... " },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "consolidate_files" },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "check_disk_space_availability" },
		{ "type" : "info", "msg" : "Downloading packs for: " },
		{ "type" : "info", "msg" : " - test-bundle " },
	EOM
	)
	expected_output2=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "download_packs" },
		{ "type" : "info", "msg" : "Finishing packs extraction... " },
		{ "type" : "info", "msg" : "No extra files need to be downloaded " },
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" },
		{ "type" : "info", "msg" : "Installing bundle(s) files... " },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 4, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 8, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 12, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 16, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 20, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 25, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 29, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 33, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 37, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 41, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 45, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 50, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 54, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 58, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 62, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 66, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 70, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 75, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 79, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 83, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 87, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 91, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 95, "stepDescription" : "install_files" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "install_files" },
		{ "type" : "info", "msg" : "Calling post-update helper scripts " },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 7, "stepCompletion" : 100, "stepDescription" : "run_scripts" },
		{ "type" : "info", "msg" : "Successfully installed 1 bundle " },
		{ "type" : "end", "section" : "bundle-add", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output1"
	assert_in_output "$expected_output2"

}
