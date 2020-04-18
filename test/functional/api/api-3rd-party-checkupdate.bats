#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 staging repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 repo1

	create_third_party_repo -a "$TEST_NAME" 10 staging repo2
	create_bundle -L -t -n test-bundle2 -f /bar/file_2 -u repo2 "$TEST_NAME"

}

@test "API009: 3rd-party checkupdate" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		20
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API010: 3rd-party checkupdate (update available) with --repo" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API011: 3rd-party checkupdate (up to date) with --repo" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS --repo repo2 --quiet"

	assert_status_is "$SWUPD_NO"
	assert_output_is_empty

}
#WEIGHT=21
