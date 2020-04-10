#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_third_party_repo -a "$TEST_NAME" 10 staging repo2
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /bar/file_2 -u repo2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 staging repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 repo1

}

@test "API014: 3rd-party update" {

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API015: 3rd-party update with --repo" {

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API016: 3rd-party update with --repo (no update available)" {

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --repo repo2 --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
