#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

}

test_setup() {

	# do nothing
	return

}

test_teardown() {

	sudo rm -rf "$TEST_NAME"/target-dir/etc/swupd

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "mirror /etc/swupd is a file" {

	sudo touch "$TEST_NAME"/target-dir/etc/swupd

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		.*/etc/swupd: not a directory
		Unable to set mirror url
		Default version URL not found. Use the -v option instead.
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "mirror /etc/swupd is a symlink to a file" {

	sudo touch "$TEST_NAME"/target-dir/foo
	sudo ln -s $(realpath "$TEST_NAME"/target-dir/foo) "$TEST_NAME"/target-dir/etc/swupd
	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		.*/etc/swupd: not a directory
		Unable to set mirror url
		Default version URL not found. Use the -v option instead.
	EOM
	)
	assert_regex_is_output "$expected_output"

}
