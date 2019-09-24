#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"

	# add test-bundle2 as a dependency of test-bundle1 and
	# test-bundle3 as optional
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3

}

@test "REM021: Removing an optional bundle" {

	# An optional bundle is not required to be installed in the system while includes
	# are, but they are installed by default. Users shuld be able to remove them.

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Removing bundle: test-bundle3
		Deleting bundle files...
		Total deleted files: 3
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/baz/test-file3
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

}
