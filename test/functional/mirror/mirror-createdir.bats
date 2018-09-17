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

	sudo rm -rf "$TEST_NAME"/target-dir/etc/swupd

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "mirror /etc/swupd does not exist" {

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
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_contenturl)
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_versionurl)

}

@test "mirror /etc/swupd already exists" {

	sudo mkdir -p "$TEST_NAME"/target-dir/etc/swupd
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
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_contenturl)
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_versionurl)

}

@test "mirror /etc/swupd is a symlink to a directory" {

	sudo mkdir "$TEST_NAME"/target-dir/foo
	sudo ln -s $(realpath "$TEST_NAME"/target-dir/foo) "$TEST_NAME"/target-dir/etc/swupd
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
  
  	! [[ -L "$TEST_NAME/target-dir/etc/swupd" ]]
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_contenturl)
	assert_equal "http://example.com/swupd-file" $(<$TEST_NAME/target-dir/etc/swupd/mirror_versionurl)

}
