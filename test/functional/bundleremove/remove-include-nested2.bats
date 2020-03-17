#!/usr/bin/env bats

load "../testlib"

export file_path

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1,/common "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /test-file3 "$TEST_NAME"
	# add dependencies
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle2
	# collect info from the common file
	file_hash=$(get_hash_from_manifest "$WEBDIR"/10/Manifest.test-bundle1 /common)
	file_path="$WEBDIR"/10/files/"$file_hash"

	create_bundle -L -n test-bundle4 -f /test-file4,/common:"$file_path" "$TEST_NAME"
	create_bundle -L -n test-bundle5 -f /test-file5,/common:"$file_path" "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle4
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle5

}

@test "REM022: Force remove a bundle that is a nested dependency of other bundles" {

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
	assert_file_exists "$TARGETDIR"/test-file4
	assert_file_exists "$TARGETDIR"/test-file5
	assert_file_exists "$TARGETDIR"/common

	# When removing a bundle that is required by other bundles, if the --force
	# option is used, it should be allowed, the specified bundle should be deleted
	# and all bundles that require it should be deleted too

	run sudo sh -c "$SWUPD bundle-remove --force $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --force option was used, the specified bundle and all bundles that require it will be removed from the system
		Bundle "test-bundle1" is required by the following bundles:
		 - test-bundle2
		 - test-bundle3
		The following bundles are being removed:
		 - test-bundle1
		 - test-bundle3
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 1 bundle
		2 bundles that depended on the specified bundle(s) were removed
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_not_exists "$TARGETDIR"/test-file1
	assert_file_not_exists "$TARGETDIR"/test-file2
	assert_file_not_exists "$TARGETDIR"/test-file3
	assert_file_exists "$TARGETDIR"/test-file4
	assert_file_exists "$TARGETDIR"/test-file5
	assert_file_exists "$TARGETDIR"/common

}
