#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

}

@test "VER037: Install only os-core" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Fix successful
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core

}
