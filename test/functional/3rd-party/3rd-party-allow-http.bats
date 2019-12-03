#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

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

@test "TPR005: Trying to add a http repo" {

	run sudo sh -c "$SWUPD 3rd-party add my_repo http://example.com/swupd-file $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR006: Forcing a http repo add" {

	run sudo sh -c "$SWUPD 3rd-party add my_repo http://example.com/swupd-file --allow-insecure-http $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Repository my_repo added successfully
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "cat $STATEDIR/3rd_party/repo.ini"
	assert_status_is "0"
	expected_output=$(cat <<-EOM
		[my_repo]
		url=http://example.com/swupd-file
	EOM
	)
	assert_is_output --identical "$expected_output"
}
