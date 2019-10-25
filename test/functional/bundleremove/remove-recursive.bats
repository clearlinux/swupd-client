#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# all bundles will be marked as "installed" but only
	# bundles test-bundle1 and test-bundle6 will be marked as "tracked"
	create_bundle -L -t -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /bat/test-file3 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /baz/test-file4 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /test-file5     "$TEST_NAME"
	create_bundle -L -t -n test-bundle6 -f /test-file6     "$TEST_NAME"
	# test-bundle1 has 4 direct dependencies: os-core, test-bundle2,
	# test-bundle4 (also-add), test-bundle6 (tracked)
	add_dependency_to_manifest -p    "$WEBDIR"/10/Manifest.test-bundle1 os-core
	add_dependency_to_manifest -p    "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -p -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle4
	add_dependency_to_manifest       "$WEBDIR"/10/Manifest.test-bundle1 test-bundle6
	# and 1 indirect dependency: test-bundle3
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle3
	# test-bundle4 is a dependency of test-bundle1, but it is also a dependency
	# test-bundle5 which is also installed
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle5 test-bundle4

	# make sure all files do exist
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle6
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle6
	assert_file_exists "$TARGETDIR"/core

}

@test "REM026: Remove a bundle and its dependencies recursively" {

	# If a bundle has dependencies and is removed using the --recursive flag,
	# the bundle and all its dependencies should be removed from the system
	# with two exceptions:
	# if the dependency is a dependency of another installed bundle
	# if the dependency was previously added by the user directly

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 --recursive"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --recursive option was used, the specified bundle and its dependencies will be removed from the system
		The following bundles are being removed:
		 - test-bundle1
		 - test-bundle2
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 9
		Successfully removed 1 bundle
		2 bundles that were installed as a dependency were removed
	EOM
	)
	assert_is_output "$expected_output"

	# bundles/files that should be deleted
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3

	# bundles/files that should not be deleted
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle6
	assert_file_exists "$STATEDIR"/bundles/test-bundle6
	assert_file_exists "$TARGETDIR"/baz/test-file4
	assert_file_exists "$TARGETDIR"/test-file5
	assert_file_exists "$TARGETDIR"/test-file6
	assert_file_exists "$TARGETDIR"/core

}

@test "REM027: Removing bundles recursively do not remove tracked bundles" {

	# test-bundle4 is a dependency of 2 bundles, but since both were selected
	# to be removed, it should be deleted too
	# test-bundle6 is a tracked bundle (manually added by user), so it should
	# not be deleted

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle5 -R"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --recursive option was used, the specified bundle and its dependencies will be removed from the system
		The following bundles are being removed:
		 - test-bundle1
		 - test-bundle2
		 - test-bundle3
		 - test-bundle4
		 - test-bundle5
		Deleting bundle files...
		Total deleted files: 14
		Successfully removed 2 bundles
		3 bundles that were installed as a dependency were removed
	EOM
	)
	assert_is_output "$expected_output"

	# bundles/files that should be deleted
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_file_not_exists "$TARGETDIR"/baz/test-file4
	assert_file_not_exists "$TARGETDIR"/test-file5

	# bundles/files that should not be deleted
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle6
	assert_file_exists "$STATEDIR"/bundles/test-bundle6
	assert_file_exists "$TARGETDIR"/test-file6
	assert_file_exists "$TARGETDIR"/core

}

@test "REM028: Remove various bundles and its dependencies recursively" {

	# test-bundle4 is a dependency of 2 bundles, but since both were selected
	# to be removed, it should be deleted too

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle5 test-bundle6 -R"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --recursive option was used, the specified bundle and its dependencies will be removed from the system
		The following bundles are being removed:
		 - test-bundle1
		 - test-bundle2
		 - test-bundle3
		 - test-bundle4
		 - test-bundle5
		 - test-bundle6
		Deleting bundle files...
		Total deleted files: 16
		Successfully removed 3 bundles
		3 bundles that were installed as a dependency were removed
	EOM
	)
	assert_is_output "$expected_output"

	# bundles/files that should be deleted
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle6
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle6
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_file_not_exists "$TARGETDIR"/baz/test-file4
	assert_file_not_exists "$TARGETDIR"/test-file5
	assert_file_not_exists "$TARGETDIR"/test-file6

	# bundles/files that should not be deleted
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core

}

@test "REM029: Remove bundles recursively assumes everything is tracked if nothing is tracked" {

	# when running bundle-remove --recursive, if the tracking directory is
	# not found or empty, bundle-remove should assume everything is tracked
	sudo rm -rf "$STATEDIR"/bundles

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 --recursive"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --recursive option was used, the specified bundle and its dependencies will be removed from the system
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 3
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

	# bundles/files that should be deleted
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/foo/test-file1

	# bundles/files that should not be deleted
	assert_file_exists "$STATEDIR"/bundles/test-bundle6
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle4
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle6
	assert_file_exists "$TARGETDIR"/core
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/bat/test-file3
	assert_file_exists "$TARGETDIR"/baz/test-file4
	assert_file_exists "$TARGETDIR"/test-file5
	assert_file_exists "$TARGETDIR"/test-file6

}
