#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "API074: mirror (same URL)" {

	run sudo sh -c "$SWUPD mirror $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API075: mirror (different URL)" {

	set_content_url "$TEST_NAME" https://some.url

	run sudo sh -c "$SWUPD mirror $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		file://$ABS_TEST_DIR/web-dir
		https://some.url
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API076: mirror --set URL" {

	run sudo sh -c "$SWUPD mirror $SWUPD_OPTS --set https://some.url --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API059: mirror --unset" {

	run sudo sh -c "$SWUPD mirror $SWUPD_OPTS --unset --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}
