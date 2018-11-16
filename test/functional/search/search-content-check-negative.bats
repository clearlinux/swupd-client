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

	run sudo sh -c "$SWUPD search $SWUPD_OPTS fake-file"

	assert_status_is 0
	assert_in_output "Searching for 'fake-file'"
	# there is going to be a whole lot of content within the line
	# above and the lines below so we are excluding those from the
	# check
	expected_output=$(cat <<-EOM
		Downloading Clear Linux manifests
		.* MB total...
		Completed manifests download.
		Search term not found.
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH010: Try searching for a file that does not exist using the full path" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS /usr/lib64/test-lib100"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for '/usr/lib64/test-lib100'
		Search term not found.
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "SRH011: Try searching for a library that does not exist" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS -l libtest-nohit"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'libtest-nohit'
		Search term not found.
	EOM
	)
	assert_is_output "$expected_output"

}

