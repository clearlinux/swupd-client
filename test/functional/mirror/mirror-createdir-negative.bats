#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "MIR004: Try setting up a mirror when /etc/swupd is a file instead of a directory" {

	sudo rm -rf "$TARGET_DIR"/etc/swupd
	sudo touch "$TARGET_DIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		.*/etc/swupd: not a directory
		Error: Unable to set mirror url
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "MIR005: Try setting up a mirror when /etc/swupd is a symlink to a file instead of a directory" {

	sudo rm -rf "$TARGET_DIR"/etc/swupd
	sudo touch "$TARGET_DIR"/foo
	sudo ln -s "$(realpath "$TARGET_DIR"/foo)" "$TARGET_DIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		.*/etc/swupd: not a directory
		Error: Unable to set mirror url
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=2
