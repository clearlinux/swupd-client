#!/usr/bin/env bats

load "../testlib"

export file_path

test_setup() {

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

}

@test "REM011: Try removing a bundle that is a nested dependency of another bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	expected_output=$(cat <<-EOM
		Bundle "test-bundle1" is required by the following bundles:
		 - test-bundle2
		 - test-bundle3
		Error: Bundle "test-bundle1" is required by 2 bundles, skipping it...
		Use "swupd bundle-remove --force test-bundle1" to remove "test-bundle1" and all bundles that require it
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
	assert_file_exists "$TARGETDIR"/common

}

@test "REM022: Force remove a bundle that is a nested dependency of other bundles" {

	create_bundle -L -n test-bundle4 -f /test-file4,/common:"$file_path" "$TEST_NAME"
	create_bundle -L -n test-bundle5 -f /test-file5,/common:"$file_path" "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle4
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle5

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

@test "REM023: Try removing multiple bundles that have a dependency of another bundle" {

	# if the user provide multilple bundles in the command they all should
	# be considered for the overall removal operation regardless of the
	# order of the bundles

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	expected_output=$(cat <<-EOM
		Bundle "test-bundle1" is required by the following bundles:
		 - test-bundle3
		Error: Bundle "test-bundle1" is required by 1 bundle, skipping it...
		Use "swupd bundle-remove --force test-bundle1" to remove "test-bundle1" and all bundles that require it
		Bundle "test-bundle2" is required by the following bundles:
		 - test-bundle3
		Error: Bundle "test-bundle2" is required by 1 bundle, skipping it...
		Use "swupd bundle-remove --force test-bundle2" to remove "test-bundle2" and all bundles that require it
		Failed to remove 2 of 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

	# re-try the test inverting the order of the bundles

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle2 test-bundle1"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	expected_output=$(cat <<-EOM
		Bundle "test-bundle2" is required by the following bundles:
		 - test-bundle3
		Error: Bundle "test-bundle2" is required by 1 bundle, skipping it...
		Use "swupd bundle-remove --force test-bundle2" to remove "test-bundle2" and all bundles that require it
		Bundle "test-bundle1" is required by the following bundles:
		 - test-bundle3
		Error: Bundle "test-bundle1" is required by 1 bundle, skipping it...
		Use "swupd bundle-remove --force test-bundle1" to remove "test-bundle1" and all bundles that require it
		Failed to remove 2 of 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

}

@test "REM024: Remove a bundle that is a nested dependency of other bundles by providing them as args" {

	# if the user provide multilple bundles in the command they all should
	# be considered for the overall removal operation regardless of the
	# order of the bundles

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle2 test-bundle1 test-bundle3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle3
		 - test-bundle1
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 7
		Successfully removed 3 bundles
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

	# reinstall the bundles
	install_bundle "$WEBDIR"/10/Manifest.test-bundle1
	install_bundle "$WEBDIR"/10/Manifest.test-bundle2
	install_bundle "$WEBDIR"/10/Manifest.test-bundle3

	# re-try again using a different order in the bundles

	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle3 test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle2
		 - test-bundle3
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 7
		Successfully removed 3 bundles
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

}

@test "REM025: Try removing a bundle that is a nested dependency of another bundle, use --verbose to see deps as a tree" {

	run sudo sh -c "$SWUPD bundle-remove --verbose $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	expected_output=$(cat <<-EOM
		Bundle "test-bundle1" is required by the following bundles:
		format:
		 # * is-required-by
		 #   |-- is-required-by
		 # * is-also-required-by
		 # ...
		  * test-bundle2
		    |-- test-bundle3
		Error: Bundle "test-bundle1" is required by 2 bundles, skipping it...
		Use "swupd bundle-remove --force test-bundle1" to remove "test-bundle1" and all bundles that require it
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
	assert_file_exists "$TARGETDIR"/common

}
