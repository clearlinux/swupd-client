#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

}

test_teardown() {

	sudo rm -rf "$TARGETDIR"/etc/swupd

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "MIR016: Set invalid parameters" {

	run sudo sh -c "$SWUPD mirror --set $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Option '--set' requires an argument
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD mirror --set url1 url2 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"
}

@test "MIR017: Unset invalid parameters" {

	run sudo sh -c "$SWUPD mirror --unset url $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD mirror --unset -c url $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Version or content url can only be used with --set
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD mirror --unset -v url $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Version or content url can only be used with --set
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD mirror --unset -u url $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Version or content url can only be used with --set
	EOM
	)
	assert_in_output "$expected_output"
}

@test "MIR018: Set/Unset invalid parameter" {

	run sudo sh -c "$SWUPD mirror --set url --unset $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: cannot set and unset at the same time
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD mirror --set url1 url2 $SWUPD_OPTS"
	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: Unexpected arguments
	EOM
	)
	assert_in_output "$expected_output"
}

#WEIGHT=2
