#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	create_bundle -n test-bundle -f /file_1,/file_2 "$TEST_NAME"
	# we will make swupd believe these bundles are already installed
	# by creating their tracking file in the system
	sudo touch "$TARGETDIR"/usr/share/clear/bundles/os-core
	sudo touch "$TARGETDIR"/usr/share/clear/bundles/test-bundle

}

@test "VER039: Install multiple bundles" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Fix successful
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/file_1
	assert_file_exists "$TARGETDIR"/file_2
	assert_file_exists "$TARGETDIR"/core

}
