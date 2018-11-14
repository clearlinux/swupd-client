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
	sudo rm "$WEBDIR"/10/pack-test-bundle-from-0.tar
	sudo rm "$WEBDIR"/10/pack-os-core-from-0.tar

}

@test "VER041: Install bundle with no zero packs should fall back to fullfiles" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

	# everything should still work
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Fix successful
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/file_1
	assert_file_exists "$TARGETDIR"/file_2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle

}
