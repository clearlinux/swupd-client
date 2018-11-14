#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10
	create_bundle -n test-bundle -f /file_1,/file_2 "$TEST_NAME"
	# note the typo on the next line
	sudo touch "$TARGETDIR"/usr/share/clear/bundles/test-bundl

}

@test "VER038: Install bundle with invalid name" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

	assert_status_is "$EMANIFEST_LOAD"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundl" is invalid, skipping it...
	EOM
	)
	assert_in_output "$expected_output"

}
