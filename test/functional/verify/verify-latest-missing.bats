#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove the formatstaging/latest
	sudo rm -rf "$WEBDIR"

}

test_teardown() {

	# we need to manually clean this up because the destroy_test_environment
	# function will refuse to delete this since the test environment does not
	# look like a test environment anymore (missing web-dir)
	sudo rm -rf "$TEST_NAME"

}

@test "verify install using latest with missing version file on server" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m latest"
	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Curl error: \(37\) Couldn.t read a file:// file
		Failed verify initialization, exiting now.
	EOM
	)
	assert_regex_is_output "$expected_output"
	
}
