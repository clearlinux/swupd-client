#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /file_2 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "REM012: Removing a bundle that has another bundle as dependency" {

	# When removing a bundle that has another bundle as dependency, only the
	# requested bundle should be deleted but its dependencies should not

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	# test-bundle1 is deleted
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/file_1
	# test-bundle2 is not deleted
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/file_2
	expected_output=$(cat <<-EOM
		Removing bundle: test-bundle1
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
