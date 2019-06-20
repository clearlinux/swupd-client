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

	sudo rm -rf "$TARGETDIR"/etc/swupd

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "MIR004: Try setting up a mirror when /etc/swupd is a file instead of a directory" {

	sudo rm -rf "$TARGETDIR"/etc/swupd
	sudo touch "$TARGETDIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file --allow-insecure-http $SWUPD_OPTS_MIRROR"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		.*/etc/swupd: not a directory
		Warning: Unable to set mirror url
		Installed version: 10
		Version URL:       file://.*/web-dir
		Content URL:       file://.*/web-dir
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "MIR005: Try setting up a mirror when /etc/swupd is a symlink to a file instead of a directory" {

	sudo rm -rf "$TARGETDIR"/etc/swupd
	sudo touch "$TARGETDIR"/foo
	sudo ln -s "$(realpath "$TARGETDIR"/foo)" "$TARGETDIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file --allow-insecure-http $SWUPD_OPTS_MIRROR"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		.*/etc/swupd: not a directory
		Warning: Unable to set mirror url
		Installed version: 10
		Version URL:       file://.*/web-dir
		Content URL:       file://.*/web-dir
	EOM
	)
	assert_regex_is_output "$expected_output"

}
