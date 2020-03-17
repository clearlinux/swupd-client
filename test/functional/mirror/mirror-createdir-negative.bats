#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"

}

@test "MIR004: Try setting up a mirror when /etc/swupd is a file instead of a directory" {

	sudo rm -rf "$TARGETDIR"/etc/swupd
	sudo touch "$TARGETDIR"/etc/swupd

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

	sudo rm -rf "$TARGETDIR"/etc/swupd
	sudo touch "$TARGETDIR"/foo
	sudo ln -s "$(realpath "$TARGETDIR"/foo)" "$TARGETDIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		.*/etc/swupd: not a directory
		Error: Unable to set mirror url
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=1
