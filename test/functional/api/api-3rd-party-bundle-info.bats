#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_third_party_repo -a "$TEST_NAME" 10 staging repo2
	create_bundle -L -t -n test-bundle1 -f /file_1a,/file_1b,/file_1c -u repo1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 -u repo2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 -u repo2 "$TEST_NAME"
	create_bundle -n test-bundle4 -f /file_4 -u repo2 "$TEST_NAME"
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle2 test-bundle3
	add_dependency_to_manifest "$TPWEBDIR"/10/Manifest.test-bundle3 test-bundle4

}

@test "API042: 3rd-party bundle-info BUNDLE" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Explicitly installed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API043: 3rd-party bundle-info BUNDLE --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --repo repo2 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Not installed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API044: 3rd-party bundle-info BUNDLE --dependencies" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --dependencies --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API045: 3rd-party bundle-info BUNDLE --dependencies --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --dependencies --repo repo2 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API046: 3rd-party bundle-info BUNDLE --files" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle1 --files --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		/usr/share/clear/bundles/test-bundle1
		/usr
		/usr/share
		/usr/share/clear
		/usr/share/clear/bundles
		/file_1c
		/file_1b
		/file_1a
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API047: 3rd-party bundle-info BUNDLE --files --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle1 --files --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		/usr/share/clear/bundles/test-bundle1
		/usr
		/usr/share
		/usr/share/clear
		/usr/share/clear/bundles
		/file_1c
		/file_1b
		/file_1a
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API048: 3rd-party bundle-info BUNDLE --dependencies --files" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --dependencies --files --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4

		/file_2
		/file_3
		/file_4
		/usr
		/usr/lib
		/usr/lib/os-release
		/usr/share
		/usr/share/clear
		/usr/share/clear/bundles
		/usr/share/clear/bundles/os-core
		/usr/share/clear/bundles/test-bundle2
		/usr/share/clear/bundles/test-bundle3
		/usr/share/clear/bundles/test-bundle4
		/usr/share/clear/update-ca
		/usr/share/clear/update-ca/Swupd_Root.pem
		/usr/share/defaults
		/usr/share/defaults/swupd
		/usr/share/defaults/swupd/format
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "API049: 3rd-party bundle-info BUNDLE --dependencies --files --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --dependencies --files --repo repo2 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle3
		test-bundle4

		/file_2
		/file_3
		/file_4
		/usr
		/usr/lib
		/usr/lib/os-release
		/usr/share
		/usr/share/clear
		/usr/share/clear/bundles
		/usr/share/clear/bundles/os-core
		/usr/share/clear/bundles/test-bundle2
		/usr/share/clear/bundles/test-bundle3
		/usr/share/clear/bundles/test-bundle4
		/usr/share/clear/update-ca
		/usr/share/clear/update-ca/Swupd_Root.pem
		/usr/share/defaults
		/usr/share/defaults/swupd
		/usr/share/defaults/swupd/format
	EOM
	)
	assert_is_output --identical "$expected_output"

}
