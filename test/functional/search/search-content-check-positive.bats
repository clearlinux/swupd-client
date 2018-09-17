#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /foo/test-file1,/usr/bin/test-bin,/usr/lib/test-lib32 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2,/usr/lib64/test-lib64 "$TEST_NAME"

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

@test "search for a binary" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS -b test-bin"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-bin'
		.*
		Bundle test-bundle	\(.* MB to install\)
		./usr/bin/test-bin
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "search for a file everywhere" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS test-bin"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-bin'
		Bundle test-bundle	\(.* MB to install\)
		./usr/bin/test-bin
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "search for a file specifying the full path" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS /usr/lib64/test-lib64"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for '/usr/lib64/test-lib64'
		Bundle test-bundle2	\(.* MB to install\)
		./usr/lib64/test-lib64
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "search for a library in lib32" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS -l test-lib32"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-lib32'
		Bundle test-bundle	\(.* MB to install\)
		./usr/lib/test-lib32
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "search for a library in lib64" {

	run sudo sh -c "$SWUPD search $SWUPD_OPTS -l test-lib64"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'test-lib64'
		Bundle test-bundle2	\(.* MB to install\)
		./usr/lib64/test-lib64
	EOM
	)
	assert_regex_is_output "$expected_output"

}
