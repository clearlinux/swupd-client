#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle3 -f /file_3 "$TEST_NAME"
	create_bundle -L -e -n test-bundle4 -f /file_4 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle4

}

@test "API023: bundle-list" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4: experimental
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API024: bundle-list --all" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
		test-bundle3
		test-bundle4: experimental
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API025: bundle-list --has-dep BUNDLE" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle4 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		test-bundle3
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API026: bundle-list --deps BUNDLE" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps test-bundle3 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle4
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API027: bundle-list --status" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --status --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core: installed
		test-bundle3: explicitly installed
		test-bundle4: installed, experimental
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API028: bundle-list --all --status" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all --status --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core: installed
		test-bundle1
		test-bundle2
		test-bundle3: explicitly installed
		test-bundle4: installed, experimental
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=5
