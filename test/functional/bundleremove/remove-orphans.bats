#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -f /file1 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /file2 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /file3 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /file4 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file5 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle4 test-bundle5

}

@test "REM030: Remove orphan bundles" {

	# User can remove all bundles that are no longer required by
	# tracked bundles

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle5
		 - test-bundle4
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 3 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM031: Try removing orphan bundles along with other bundles" {

	# User cannot provide other bundles to be removed when using the
	# --orphans flag since this could potentially change the list of
	# orphans

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 --orphans"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: you cannot specify bundles to remove when using --orphans
	EOM
	)
	assert_in_output "$expected_output"

}
