#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	REPO1="$TP_WEB_DIR"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle1 -f /file_1 -u repo1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /file_2 -u repo1 "$TEST_NAME"
	create_bundle       -n test-bundle3 -f /file_3 -u repo1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle4 -f /file_4 -u repo2 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file_5 -u repo2 "$TEST_NAME"
	add_dependency_to_manifest "$REPO1"/10/Manifest.test-bundle1 test-bundle2

}

@test "API029: 3rd-party bundle-list" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		os-core
		test-bundle1
		test-bundle2
		[repo2]
		os-core
		test-bundle4
		test-bundle5
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API030: 3rd-party bundle-list --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API031: 3rd-party bundle-list --all" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --all --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		os-core
		test-bundle1
		test-bundle2
		test-bundle3
		[repo2]
		os-core
		test-bundle4
		test-bundle5
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API032: 3rd-party bundle-list --all --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --all --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
		test-bundle3
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API033: 3rd-party bundle-list --has-dep BUNDLE" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --has-dep test-bundle2 --quiet"

	assert_status_is "$SWUPD_BUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		[repo1]
		test-bundle1
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API034: 3rd-party bundle-list --has-dep BUNDLE --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --has-dep test-bundle2 --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		test-bundle1
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API035: 3rd-party bundle-list --deps BUNDLE" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --deps test-bundle1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		os-core
		test-bundle2
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API036: 3rd-party bundle-list --deps BUNDLE --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --deps test-bundle1 --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle2
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API078: 3rd-party bundle-list --orphans" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --orphans --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		[repo2]
		test-bundle5
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API079: 3rd-party bundle-list --orphans --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party bundle-list $SWUPD_OPTS --orphans --repo repo2 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		test-bundle5
	EOM
	)
	assert_is_output "$expected_output"

}

#WEIGHT=10
