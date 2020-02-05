#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a 3rd-party repo within the test environment and add
	# a few bundles in the 3rd-party repo
	add_third_party_repo "$TEST_NAME" 10
	create_bundle -n test-bundle1 -f /foo/file_1 -u test-repo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/file_2 -u test-repo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3     -u test-repo "$TEST_NAME"

	# add test-bundle2 as a dependency of test-bundle1 and test-bundle3 as optional
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -o "$TPWEBDIR"/10/Manifest.test-bundle1 test-bundle3

	create_config_file

}

@test "TPR015: Set a local option in a command that uses a subcommand in the configuration file" {

	# some commands can include subcommands, in those cases the command and
	# subcommand are represented by a section like [command-subcommand] in the
	# config file

	add_option_to_config_file skip_optional true 3rd-party-bundle-add

	cd "$TEST_NAME" || exit 1
	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"
	cd - || exit 1

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
		1 bundle was installed as dependency
	EOM
	)
	assert_is_output "$expected_output"

	# test-bundle1 is installed and tracked
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TPSTATEDIR"/bundles/test-bundle1

	# test-bundle2 is installed but not tracked
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TPSTATEDIR"/bundles/test-bundle2

	# test-bundle3 is not installed at all
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo/usr/share/clear/bundles/test-bundle3
	assert_file_not_exists "$TPSTATEDIR"/bundles/test-bundle3

}
