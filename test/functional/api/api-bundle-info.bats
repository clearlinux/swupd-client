#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -f /file_1a,/file_1b,/file_1c "$TEST_NAME"
	create_bundle -e -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 "$TEST_NAME"
	create_bundle -n test-bundle4 -f /file_4 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle3
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle4

}

@test "API037: bundle-info BUNDLE (installed)" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Explicitly installed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API038: bundle-info BUNDLE (not installed)" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle2 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Not installed, experimental
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API039: bundle-info BUNDLE --dependencies" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle2 --dependencies --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API040: bundle-info BUNDLE --files" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --files --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		/usr/share/clear/bundles/test-bundle1
		/file_1c
		/file_1b
		/file_1a
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API041: bundle-info BUNDLE --dependencies --files" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle2 --dependencies --files --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4

		/core
		/file_2
		/file_3
		/file_4
		/usr/share/clear/bundles/os-core
		/usr/share/clear/bundles/test-bundle2
		/usr/share/clear/bundles/test-bundle3
		/usr/share/clear/bundles/test-bundle4
	EOM
	)
	assert_is_output --identical "$expected_output"

}
