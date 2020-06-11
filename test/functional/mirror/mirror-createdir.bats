#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

}

@test "MIR001: Setting a mirror when /etc/swupd doesn't exist" {

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

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

}

@test "MIR002: Setting a mirror when /etc/swupd already exist" {

	sudo mkdir -p "$TARGET_DIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

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

}

@test "MIR003: Setting a mirror when /etc/swupd is a symlink to a directory" {

	sudo mkdir "$TARGET_DIR"/foo
	sudo ln -s "$(realpath "$TARGET_DIR"/foo)" "$TARGET_DIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s https://example.com/swupd-file $SWUPD_OPTS"

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

	! [[ -L "$TARGET_DIR/etc/swupd" ]]
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGET_DIR"/etc/swupd/mirror_versionurl)"

}
#WEIGHT=2
