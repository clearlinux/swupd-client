#!/usr/bin/env bats

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

@test "MIR001: Setting a mirror when /etc/swupd doesn't exist" {

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"

	assert_status_is 0
	expected_output=$(cat <<-EOM
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

@test "MIR002: Setting a mirror when /etc/swupd already exist" {

	sudo mkdir -p "$TARGETDIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"

	assert_status_is 0
	expected_output=$(cat <<-EOM
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

@test "MIR003: Setting a mirror when /etc/swupd is a symlink to a directory" {

	sudo mkdir "$TARGETDIR"/foo
	sudo ln -s "$(realpath "$TARGETDIR"/foo)" "$TARGETDIR"/etc/swupd

	run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Set upstream mirror to http://example.com/swupd-file
		Installed version: 10
		Version URL:       http://example.com/swupd-file
		Content URL:       http://example.com/swupd-file
	EOM
	)
	assert_is_output "$expected_output"
  
  	! [[ -L "$TARGETDIR/etc/swupd" ]]
	assert_equal "http://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_contenturl)"
	assert_equal "http://example.com/swupd-file" "$(<"$TARGETDIR"/etc/swupd/mirror_versionurl)"

}
