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

@test "search with non existant file, specifying full path" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS /usr/lib64/test-lib100"
	assert_status_is 0
	assert_in_output "Searching for '/usr/lib64/test-lib100'"
	expected_output=$(cat <<-EOM
		Downloading Clear Linux manifests
		   .* MB total\.\.\.
		Completed manifests download\.
		Search term not found\.
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "search with non existant library" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS -l libtest-nohit"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'libtest-nohit'
		Search term not found.
	EOM
	)
	assert_is_output "$expected_output"

}

