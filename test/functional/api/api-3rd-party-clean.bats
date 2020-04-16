#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	STATE1="$TPSTATEDIR"
	sudo mkdir "$STATE1"/10
	sudo touch "$STATE1"/10/Manifest.test{1..3}
	sudo touch "$STATE1"/pack-test{1..2}-from-0.tar
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	STATE2="$TPSTATEDIR"
	sudo touch "$STATE2"/pack-test3-from-0.tar

}


@test "API065: 3rd-party clean" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API066: 3rd-party clean --dry-run" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --dry-run --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		\\[repo1\\]
		$TEST_ROOT_DIR/$STATE1/pack-test.-from-0.tar
		$TEST_ROOT_DIR/$STATE1/pack-test.-from-0.tar
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
		\\[repo2\\]
		$TEST_ROOT_DIR/$STATE2/pack-test3-from-0.tar
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "API067: 3rd-party clean --dry-run --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --dry-run --quiet --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$TEST_ROOT_DIR/$STATE1/pack-test.-from-0.tar
		$TEST_ROOT_DIR/$STATE1/pack-test.-from-0.tar
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
		$TEST_ROOT_DIR/$STATE1/10/Manifest.test.
	EOM
	)
	assert_regex_is_output "$expected_output"

}
