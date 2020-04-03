#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

	# create a 3rd-party repo with bundles that have dependencies
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle  -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"
	create_bundle  -n test-bundle2 -f /bar/file_2 -u repo1 "$TEST_NAME"
	create_bundle  -n test-bundle3 -f /baz/file_3 -u repo1 "$TEST_NAME"
	create_bundle  -n test-bundle4 -f /file_4     -u repo1 "$TEST_NAME"
	create_bundle  -n test-bundle5 -f /bat/file_5 -u repo1 "$TEST_NAME"
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle2 test-bundle3
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle3 test-bundle4
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle2 test-bundle5

}

@test "TPR020: List dependencies of a 3rd-party bundle" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --deps test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________
		 3rd-Party Repo: repo1
		_______________________

		Loading required manifests...

		Bundles included by test-bundle2:
		 - test-bundle3
		 - test-bundle4
		 - test-bundle5

		Total: 3
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "TPR021: List all 3rd-party bundles that have a given bundle as dependency" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --has-dep test-bundle4 --all"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________
		 3rd-Party Repo: repo1
		_______________________

		Loading required manifests...

		All bundles that have test-bundle4 as a dependency:
		 - test-bundle2
		 - test-bundle3

		Total: 2
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=8
