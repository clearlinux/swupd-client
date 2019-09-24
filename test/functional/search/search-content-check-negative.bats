#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /foo/test-file1,/usr/lib/test-lib32,/libtest-nohit "$TEST_NAME"

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

@test "SRH009: Try searching for a file that does not exist" {

	# the first time we run search it needs to download the
	# manifests, so we need to account for those messages in
	# this first test

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS fake-file"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'fake-file'
		Search term not found
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SRH010: Try searching for a file that does not exist using the full path" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS /usr/lib64/test-lib100"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for '/usr/lib64/test-lib100'
		Search term not found
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH011: Try searching for a library that does not exist" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS -l libtest-nohit"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'libtest-nohit'
		Search term not found
	EOM
	)
	assert_is_output "$expected_output"

}

