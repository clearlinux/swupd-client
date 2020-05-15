#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	STATE1="$TPSTATEDIR_ABS"
	sudo mkdir -p "$TPSTATEDIR_MANIFEST"/10
	sudo touch "$TPSTATEDIR_MANIFEST"/10/Manifest.test{1..3}
	sudo touch "$TPSTATEDIR_CACHE"/pack-test{1..2}-from-0.tar
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	STATE2="$TPSTATEDIR_ABS"
	sudo touch "$TPSTATEDIR_CACHE"/pack-test3-from-0.tar

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
		$STATE1/cache/pack-test.-from-0.tar
		$STATE1/cache/pack-test.-from-0.tar
		$STATE1/cache/manifest/10/Manifest.test.
		$STATE1/cache/manifest/10/Manifest.test.
		$STATE1/cache/manifest/10/Manifest.test.
		\\[repo2\\]
		$STATE2/cache/pack-test3-from-0.tar
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "API067: 3rd-party clean --dry-run --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --dry-run --quiet --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$STATE1/cache/pack-test.-from-0.tar
		$STATE1/cache/pack-test.-from-0.tar
		$STATE1/cache/manifest/10/Manifest.test.
		$STATE1/cache/manifest/10/Manifest.test.
		$STATE1/cache/manifest/10/Manifest.test.
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=13
