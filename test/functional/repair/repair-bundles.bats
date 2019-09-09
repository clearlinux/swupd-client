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

@test "REP033: Repair bundles only fixes specified bundles" {

	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/common
	sudo rm -f "$TARGETDIR"/foo/test-file3

	# users can diagnose only specific bundles

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle1,test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Limiting diagnose to the following bundles:
		 - test-bundle1
		 - test-bundle2
		Download missing manifests ...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $PATH_PREFIX/common -> fixed
		 -> Missing file: $PATH_PREFIX/foo/test-file1 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 7 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP034: Repair bundles requires --force to continue when there is an invalid bundle" {

	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/common
	write_to_protected_file -a "$TARGETDIR"/usr/share/clear/bundles/test-bundle1 "something"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles bad-name,test-bundle1"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "bad-name" is invalid or no longer available
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Download missing manifests ...
		Error: One or more of the provided bundles are not available at version 10
		Please make sure the name of the provided bundles are correct, or use --force to override
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles bad-name,test-bundle1 --force"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "bad-name" is invalid or no longer available
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Download missing manifests ...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $PATH_PREFIX/common -> fixed
		 -> Missing file: $PATH_PREFIX/foo/test-file1 -> fixed
		Repairing corrupt files
		 -> Hash mismatch for file: $PATH_PREFIX/usr/share/clear/bundles/test-bundle1 -> fixed
		Removing extraneous files
		Inspected 4 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP035: Repair bundles do not delete files needed by other bundles" {

	update_manifest "$WEBDIR"/10/Manifest.test-bundle1 file-status /foo/test-file1 .d..
	update_manifest "$WEBDIR"/10/Manifest.test-bundle1 file-status /common .d..
	write_to_protected_file -a "$TARGETDIR"/usr/share/clear/bundles/test-bundle1 "something"

	# if a file is marked as deleted in a bundle being repaired in isolation
	# but another bundle needs that file, it should not be deleted

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle1,test-bundle3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Limiting diagnose to the following bundles:
		 - test-bundle1
		 - test-bundle3
		Download missing manifests ...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Repairing corrupt files
		 -> Hash mismatch for file: $PATH_PREFIX/usr/share/clear/bundles/test-bundle1 -> fixed
		Removing extraneous files
		 -> File that should be deleted: $PATH_PREFIX/foo/test-file1 -> deleted
		Inspected 7 files
		  1 file did not match
		    1 of 1 files were repaired
		    0 of 1 files were not repaired
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REP036: Repair a list of bundles that include a bundle not installed" {

	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/common

	# if a user repairs a list of bundles that include one not installed,
	# it should warn the user about it, it should ignore the uninstalled bundle
	# and continue

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --bundles test-bundle1,test-bundle4"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Warning: Bundle "test-bundle4" is not installed, skipping it...
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Download missing manifests ...
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		 -> Missing file: $PATH_PREFIX/common -> fixed
		 -> Missing file: $PATH_PREFIX/foo/test-file1 -> fixed
		Repairing corrupt files
		Removing extraneous files
		Inspected 4 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"

}
