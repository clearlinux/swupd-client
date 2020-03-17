#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"
	create_bundle -n test-bundle4 -f /bat/test-file4 "$TEST_NAME"
	# add test-bundle2 as a dependency of test-bundle1 and test-bundle3 as optional
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3
	# add circular dependency for also-add
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle3 test-bundle4
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle4 test-bundle3

}

@test "ADD051: Adding a bundle that includes an optional bundle" {

	# An optional bundle is not required to be installed in the system while includes
	# are. Swupd will install optional bundles on bundle-add unless specified otherwise
	# by using the --skip--optional flag.
	# circular reference between also-add should be allowed

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
		3 bundles were installed as dependencies
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$TARGETDIR"/baz/test-file3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

}

@test "ADD052: Adding a bundle skipping its optional bundle" {

	# An optional bundle is not required to be installed in the system while includes
	# are. Swupd will install optional bundles on bundle-add unless specified otherwise
	# by using the --skip--optional flag.

	run sudo sh -c "$SWUPD bundle-add --skip-optional $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
		1 bundle was installed as dependency
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/baz/test-file3
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3

}
#WEIGHT=8
