#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -e -n test-bundle1 -f /common,/foo/test-file1,/foo/test-file3,/usr/bin/test-bin,/usr/lib/test-lib32 "$TEST_NAME"
	create_bundle -L -e -n test-bundle2 -f /common,/bar/test-file2,/bar/test-file3,/usr/lib64/test-lib64 "$TEST_NAME"

}

test_teardown() {

	# do nothing
	return

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "SRH015: Search for an experimental bundle" {

	# it should find the bundle since the tracking file has the
	# same name as the bundle, also the first time we run search
	# it needs to download the manifests, so we need to account
	# for those messages. If the bundle is experimental it should
	# swhow that

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'test-bundle1'
	EOM
	)
	assert_in_output "$expected_output"

}

@test "SRH016: Search for an experimental bundle that is already installed" {

	# it should find the bundle since the tracking file has the
	# same name as the bundle. If the bundle is experimental it
	# should swhow that

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-bundle2"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'test-bundle2'
		Bundle test-bundle2 \\(experimental\\) \\[installed\\] \\(0 MB on system\\)
		./usr/share/clear/bundles/test-bundle2
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH017: Search for a file that is part of an experimental bundle" {

	# If the bundle is experimental it should swhow that

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-file1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-file1'
		Bundle test-bundle1 \\(experimental\\) \\(0 MB to install\\)
		./foo/test-file1
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH018: Search for a binary file that is part of an experimental bundle" {

	# If the bundle is experimental it should swhow that

	run sudo sh -c "$SWUPD search-file --binary $SWUPD_OPTS test-bin"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-bin'
		Bundle test-bundle1 \\(experimental\\) \\(0 MB to install\\)
		./usr/bin/test-bin
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH019: Search for a library that is part of an experimental bundle" {

	# If the bundle is experimental it should swhow that

	run sudo sh -c "$SWUPD search-file --library $SWUPD_OPTS test-lib32"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-lib32'
		Bundle test-bundle1 \\(experimental\\) \\(0 MB to install\\)
		./usr/lib/test-lib32
	EOM
	)
	assert_regex_in_output "$expected_output"

}
#WEIGHT=4
