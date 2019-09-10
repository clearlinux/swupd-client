#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -n os-core -f /core "$TEST_NAME"

}

@test "INS013: OS install using machine readable output" {

	# the --json-output flag can be used so all the output of the os-install
	# command is created as a JSON stream that can be read and parsed by other
	# applications interested into knowing real time status of the command

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output1=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "verify" },
		{ "type" : "progress", "currentStep" : 1, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "get_versions" },
		{ "type" : "info", "msg" : "Installing OS version 10 (latest) " },
		{ "type" : "progress", "currentStep" : 2, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "cleanup_download_dir" },
		{ "type" : "info", "msg" : " Download missing manifests " },
		{ "type" : "progress", "currentStep" : 3, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "load_manifests" },
		{ "type" : "progress", "currentStep" : 4, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "consolidate_files" },
		{ "type" : "info", "msg" : "Downloading packs for: " },
		{ "type" : "info", "msg" : " - os-core " },
	EOM
	)
	expected_output2=$(cat <<-EOM
		{ "type" : "progress", "currentStep" : 5, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "download_packs" },
		{ "type" : "info", "msg" : "Finishing packs extraction... " },
		{ "type" : "info", "msg" : "Checking for corrupt files " },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 8, "stepCompletion" : 50, "stepDescription" : "check_files_hash" },
		{ "type" : "progress", "currentStep" : 6, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "check_files_hash" },
		{ "type" : "info", "msg" : "No extra files need to be downloaded " },
		{ "type" : "progress", "currentStep" : 7, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "download_fullfiles" },
		{ "type" : "info", "msg" : " Installing base OS and selected bundles " },
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 8, "stepCompletion" : 50, "stepDescription" : "add_missing_files" },
		{ "type" : "progress", "currentStep" : 8, "totalSteps" : 8, "stepCompletion" : 100, "stepDescription" : "add_missing_files" },
		{ "type" : "info", "msg" : " Inspected 2 files " },
		{ "type" : "info", "msg" : "  2 files were missing " },
		{ "type" : "info", "msg" : "    2 of 2 missing files were installed " },
		{ "type" : "info", "msg" : "    0 of 2 missing files were not installed " },
		{ "type" : "info", "msg" : "Calling post-update helper scripts " },
		{ "type" : "warning", "msg" : "helper script ($TEST_DIRNAME/testfs/target-dir//usr/bin/clr-boot-manager) not found, it will be skipped " },
		{ "type" : "info", "msg" : " Installation successful " },
		{ "type" : "end", "section" : "verify", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output1"
	assert_in_output "$expected_output2"

}
