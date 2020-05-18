#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1

	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	CACHE1="$TPSTATEDIR_CACHE"
	sudo mkdir -p "$TPSTATEDIR_MANIFEST"/10
	sudo touch "$TPSTATEDIR_MANIFEST"/10/Manifest.test{1..3}
	sudo touch "$CACHE1"/pack-test{1..2}-from-0.tar

	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	CACHE2="$TPSTATEDIR_CACHE"
	sudo touch "$CACHE2"/pack-test3-from-0.tar

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
		$CACHE1/pack-test.-from-0.tar
		$CACHE1/pack-test.-from-0.tar
		$CACHE1/manifest/10/Manifest.test.
		$CACHE1/manifest/10/Manifest.test.
		$CACHE1/manifest/10/Manifest.test.
		\\[repo2\\]
		$CACHE2/pack-test3-from-0.tar
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "API067: 3rd-party clean --dry-run --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --dry-run --quiet --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$CACHE1/pack-test.-from-0.tar
		$CACHE1/pack-test.-from-0.tar
		$CACHE1/manifest/10/Manifest.test.
		$CACHE1/manifest/10/Manifest.test.
		$CACHE1/manifest/10/Manifest.test.
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=13
