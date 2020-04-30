#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	WEBDIR="$TPWEBDIR"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle1 -f /file_1 -u repo1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /file_2 -u repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /file_3 -u repo1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle4 -f /file_4 -u repo2 "$TEST_NAME"
	create_bundle       -n test-bundle5 -f /file_5 -u repo2 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "TPR093: List orphaned 3rd-party bundles" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Loading required manifests...
		Orphan bundles:
		 - test-bundle3
		Total: 1
		Use "swupd 3rd-party bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
		_____________________________
		 3rd-Party Repository: repo2
		_____________________________
		Loading required manifests...
		Orphan bundles:
		Total: 0
		Use "swupd 3rd-party bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR094: List orphaned 3rd-party bundles with --repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --orphans --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Orphan bundles:
		 - test-bundle3
		Total: 1
		Use "swupd 3rd-party bundle-add BUNDLE" to no longer list BUNDLE and its dependencies as orphaned
	EOM
	)
	assert_is_output "$expected_output"

}
