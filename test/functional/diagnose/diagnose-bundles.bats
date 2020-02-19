#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1,/common "$TEST_NAME"
	file_hash=$(get_hash_from_manifest "$WEBDIR"/10/Manifest.test-bundle1 /common)
	file_path="$WEBDIR"/10/files/"$file_hash"
	create_bundle -L -n test-bundle2 -f /bar/test-file2,/common:"$file_path" "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /baz/test-file3,/common:"$file_path" "$TEST_NAME"
	create_bundle -n test-bundle4 -f /bat/test-file4,/common:"$file_path" "$TEST_NAME"

}

@test "DIA017: Diagnose bundles only checks specified bundles" {

	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/common
	sudo rm -f "$TARGETDIR"/foo/test-file3

	# users can diagnose only specific bundles

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles test-bundle1,test-bundle2"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Limiting diagnose to the following bundles:
		 - test-bundle1
		 - test-bundle2
		Downloading missing manifests...
		Checking for missing files
		 -> Missing file: $PATH_PREFIX/common
		 -> Missing file: $PATH_PREFIX/foo/test-file1
		Checking for corrupt files
		Checking for extraneous files
		Inspected 7 files
		  2 files were missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA018: Diagnose bundles requires --force to continue when there is an invalid bundle" {

	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/common

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles bad-name,test-bundle1"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "bad-name" is invalid or no longer available
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Downloading missing manifests...
		Error: One or more of the provided bundles are not available at version 10
		Please make sure the name of the provided bundles are correct, or use --force to override
		Diagnose did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles bad-name,test-bundle1 --force"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "bad-name" is invalid or no longer available
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Downloading missing manifests...
		Checking for missing files
		 -> Missing file: $PATH_PREFIX/common
		 -> Missing file: $PATH_PREFIX/foo/test-file1
		Checking for corrupt files
		Checking for extraneous files
		Inspected 4 files
		  2 files were missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA019: Diagnose bundles do not report deleted files needed by other bundles" {

	update_manifest "$WEBDIR"/10/Manifest.test-bundle1 file-status /foo/test-file1 .d..
	update_manifest "$WEBDIR"/10/Manifest.test-bundle1 file-status /common .d..

	# if a file is marked as deleted in a bundle being diagnosed in isolation
	# but another bundle needs that file, it should not be reported as extraneous

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles test-bundle1,test-bundle3"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Limiting diagnose to the following bundles:
		 - test-bundle1
		 - test-bundle3
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		 -> File that should be deleted: $PATH_PREFIX/foo/test-file1
		Inspected 7 files
		  1 file found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA020: Try diagnosing a bundle that is not installed" {

	# if a user tries to diagnose a bundle that is not installed,
	# it should warn the user about it

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles test-bundle4"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "test-bundle4" is not installed, skipping it...
		No valid bundles were specified, nothing to be done
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA021: Diagnose a list of bundles that include a bundle not installed" {

	# if a user tries to diagnose a list that includes a bundle that is not installed,
	# it should warn the user about it, it should ignore it and continue

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles test-bundle4,test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "test-bundle4" is not installed, skipping it...
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Inspected 4 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=18
