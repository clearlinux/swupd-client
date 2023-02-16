#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

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
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl
}

@test "MIR012: Unsetting a mirror not set" {

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		No mirror url configuration to remove
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl
}

@test "MIR013: Set/Unset a full mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -u https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Overriding version and content URLs with https://example.com/swupd-file
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       https://example.com/swupd-file
		Content URL:       https://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl
}

@test "MIR014: Set/Unset a content mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -c https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Overriding content URL with https://example.com/swupd-file
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       https://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_contenturl)"
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl
}

@test "MIR015: Set/Unset a version mirror with globals" {

	run sudo sh -c "$SWUPD mirror --set -v https://example.com/swupd-file $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Overriding version URL with https://example.com/swupd-file
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       https://example.com/swupd-file
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_versionurl)"

	run sudo sh -c "$SWUPD mirror --unset $SWUPD_OPTS"
	assert_status_is 0

	expected_output=$(cat <<-EOM
		Mirror url configuration removed
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$ABS_TEST_DIR/web-dir
		Content URL:       file://$ABS_TEST_DIR/web-dir
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_contenturl
	assert_file_not_exists "$TARGET_DIR"/etc/swupd/mirror_versionurl
}
#WEIGHT=2
