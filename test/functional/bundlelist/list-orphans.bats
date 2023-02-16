#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle       -n test-bundle1 -f /file1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /file2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle3 -f /file3 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /file4 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file5 "$TEST_NAME"
	create_bundle -L    -n test-bundle6 -f /file6 "$TEST_NAME"
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle3 test-bundle4

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST027: List orphaned bundles" {

	# list bundles that are orphans

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle5
		 - test-bundle6
		Total: 2
		Use "swupd bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
	EOM
	)
	assert_is_output "$expected_output"

}

@test "LST028: List orphaned bundles with status" {

	# list bundles that are orphans

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle5 (installed)
		 - test-bundle6 (installed)
		Total: 2
		Use "swupd bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
	EOM
	)
	assert_is_output "$expected_output"

}
