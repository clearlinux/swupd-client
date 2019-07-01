#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

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

@test "MIR007: Trying to set a http mirror" {

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "MIR008: Forcing a http mirror ang checking if the mirror works" {

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file --allow-insecure-http $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Set upstream mirror to http://example.com/swupd-file
		Installed version: 10
		Version URL:       http://example.com/swupd-file
		Content URL:       http://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "http://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "http://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"
}

@test "MIR009: swupd operations when http mirror is used without allow-insecure-http" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS -u http://cdn.download.clearlinux.org/update"

	assert_status_is "$SWUPD_CURL_INIT_FAILED"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
	EOM
	)
	assert_in_output "$expected_output"
}

@test "MIR010: swupd operations when http mirror is used without allow-insecure-http" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --allow-insecure-http -u http://cdn.download.clearlinux.org/update"

	# Error is because server doesn't respond to this manifest, but connection was created
	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
	EOM
	)
	assert_in_output "$expected_output"
}
