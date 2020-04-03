#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

}

@test "MIR011: Set/Unset a full mirror" {

	run sudo sh -c "$SWUPD mirror --set https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       https://example.com/swupd-file
		Content URL:       https://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl
}

@test "MIR012: Unsetting a mirror not set" {

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		No mirror url configuration to remove
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl
}

@test "MIR013: Set/Unset a full mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -u https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       https://example.com/swupd-file
		Content URL:       https://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl
}

@test "MIR014: Set/Unset a content mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -c https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       https://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl
}

@test "MIR015: Set/Unset a version mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -v https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       https://example.com/swupd-file
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGETDIR"/etc/swupd/mirror_versionurl
}
#WEIGHT=2
