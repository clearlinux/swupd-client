#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /common,/foo/test-file1,/foo/test-file3,/usr/bin/test-bin,/usr/lib/test-lib32 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /common,/bar/test-file2,/bar/test-file3,/usr/lib64/test-lib64 "$TEST_NAME"

}

test_setup() {

	# do nothing
	return

}

test_teardown() {

	# do nothing
	return

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "SRH001: Search for a bundle" {

	# it should find the bundle since the tracking file has the
	# same name as the bundle, also the first time we run search
	# it needs to download the manifests, so we need to account
	# for those messages in this first test

	run sudo sh -c "$SWUPD search $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-bundle1'
		Downloading Clear Linux manifests
		.* MB total...
		Completed manifests download.
		Bundle test-bundle1	\\(0 MB to install\\)
		./usr/share/clear/bundles/test-bundle1
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH002: Search for a file" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS test-file2"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-file2'
		Bundle test-bundle2	\\(0 MB to install\\)
		./bar/test-file2
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH003: Search for a file that is in multiple bundles" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS common"

	assert_status_is 0
	# search finds the files in different order, sometimes, test-bundle1
	# will be listed first and sometimes test-bundle2 will be listed first
	# so we need to check them separatelly
	expected_output=$(cat <<-EOM
		Bundle test-bundle1	\\(0 MB to install\\)
		./common
	EOM
	)
	assert_regex_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		Bundle test-bundle2	\\(0 MB to install\\)
		./common
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH004: Search for a file that is in multiple bundles in different locations" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS test-file3"

	assert_status_is 0
	# search finds the files in different order, sometimes, test-bundle1
	# will be listed first and sometimes test-bundle2 will be listed first
	# so we need to check them separatelly
	expected_output=$(cat <<-EOM
		Bundle test-bundle1	\\(0 MB to install\\)
		./foo/test-file3
	EOM
	)
	assert_regex_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		Bundle test-bundle2	\\(0 MB to install\\)
		./bar/test-file3
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH005: Search for a file specifying the full path" {

	# test-file3 is in two bundles but in different path, since we
	# are specifying a path, search should only find one

	run sudo sh -c "$SWUPD search $SWUPD_OPTS /bar/test-file3"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for '/bar/test-file3'
		Bundle test-bundle2	\\(0 MB to install\\)
		./bar/test-file3
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH006: Search for a binary" {

	run sudo sh -c "$SWUPD search --binary $SWUPD_OPTS test-bin"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-bin'
		Bundle test-bundle1	\\(0 MB to install\\)
		./usr/bin/test-bin
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH007: Search for a library (lib32)" {

	run sudo sh -c "$SWUPD search --library $SWUPD_OPTS test-lib32"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-lib32'
		Bundle test-bundle1	\\(0 MB to install\\)
		./usr/lib/test-lib32
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH008: Search for a library (lib 64)" {

	run sudo sh -c "$SWUPD search --library $SWUPD_OPTS test-lib64"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-lib64'
		Bundle test-bundle2	\\(0 MB to install\\)
		./usr/lib64/test-lib64
	EOM
	)
	assert_regex_is_output "$expected_output"

}
