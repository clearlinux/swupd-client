#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

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
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

}

@test "MIR002: Setting a mirror when /etc/swupd already exist" {

	sudo mkdir -p "$TARGETDIR"/etc/swupd

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
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

}

@test "MIR003: Setting a mirror when /etc/swupd is a symlink to a directory" {

	sudo mkdir "$TARGETDIR"/foo
	sudo ln -s "$(realpath "$TARGETDIR"/foo)" "$TARGETDIR"/etc/swupd

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

	! [[ -L "$TARGETDIR/etc/swupd" ]]
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "https://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

}
#WEIGHT=2
