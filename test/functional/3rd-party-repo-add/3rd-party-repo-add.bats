#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

@test "TRA001: Add a single repo" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 http://xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "cat $STATEDIR/3rd_party/repo.ini"
	expected_output=$(cat <<-EOM
			[test-repo1]
			url=http://xyz.com
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TRA002: Add multiple repos in repo config" {

	# Add 5 repos in a row one by one
	run sudo sh -c "$SWUPD 3rd-party add test-repo1 https://xyz.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo1 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo2 file://abc.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo2 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo3 https://efg.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo3 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo4 http://hij.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo4 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party add test-repo5 https://klm.com $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository test-repo5 added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "cat $STATEDIR/3rd_party/repo.ini"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM

			[test-repo1]

			url=https://xyz.com

			[test-repo2]

			url=file://abc.com

			[test-repo3]

			url=https://efg.com

			[test-repo4]

			url=http://hij.com

			[test-repo5]

			url=https://klm.com
	EOM
	)
	assert_is_output --identical "$expected_output"

}
