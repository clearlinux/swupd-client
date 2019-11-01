#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "THP001: Add a single repo, remove single repo inbetween listing" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 www.xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo1"

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repo
			--------
			name:	test-repo1
			url:	www.xyz.com
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party remove test-repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Match found for repository: test-repo1
			Repository test-repo1 and its contents removed successfully
	EOM
	)
	assert_is_output "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test-repo1"

	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

}

@test "THP002: Add multiple repos, remove at different positions in repo config" {

	# Add 5 repos in a row one by one
	run sudo sh -c "$SWUPD 3rd-party add test-repo1 www.xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo2 www.abc.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo2 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo3 www.efg.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo3 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo4 www.hij.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo4 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo5 www.klm.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo5 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	# Existence check part1
	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repo
			--------
			name:	test-repo1
			url:	www.xyz.com

			Repo
			--------
			name:	test-repo2
			url:	www.abc.com

			Repo
			--------
			name:	test-repo3
			url:	www.efg.com

			Repo
			--------
			name:	test-repo4
			url:	www.hij.com

			Repo
			--------
			name:	test-repo5
			url:	www.klm.com

	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo1"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo2"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo3"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo4"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo5"

	# remove first one
	run sudo sh -c "$SWUPD 3rd-party remove test-repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Match found for repository: test-repo1
			Repository test-repo1 and its contents removed successfully
	EOM
	)
	assert_is_output "$expected_output"

	# remove last one
	run sudo sh -c "$SWUPD 3rd-party remove test-repo5 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Match found for repository: test-repo5
			Repository test-repo5 and its contents removed successfully
	EOM
	)
	assert_is_output "$expected_output"

	# Existence check part2: middle three should exist
	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repo
			--------
			name:	test-repo2
			url:	www.abc.com

			Repo
			--------
			name:	test-repo3
			url:	www.efg.com

			Repo
			--------
			name:	test-repo4
			url:	www.hij.com

	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test-repo1"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo2"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo3"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo4"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test-repo5"

	# remove middle
	run sudo sh -c "$SWUPD 3rd-party remove test-repo3 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Match found for repository: test-repo3
			Repository test-repo3 and its contents removed successfully
	EOM
	)
	assert_is_output "$expected_output"

	# Existence check part3: only test-repo2, test-repo4 should exist
	run sudo sh -c "$SWUPD 3rd-party list $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repo
			--------
			name:	test-repo2
			url:	www.abc.com

			Repo
			--------
			name:	test-repo4
			url:	www.hij.com

	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo2"
	assert_dir_exists "$PATH_PREFIX/opt/3rd-party/test-repo4"
	assert_dir_not_exists "$PATH_PREFIX/opt/3rd-party/test-repo3"
}
