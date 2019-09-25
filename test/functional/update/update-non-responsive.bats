#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# Skip this test if not running in Travis CI, because test takes too long for
	# local development.
	# To run this locally along with all other tests do:
	#    TRAVIS=true make check
	# or to run only this test locally do:
	#    TRAVIS=true bats update-non-responsive.bats
	if [ -z "$TRAVIS" ]; then
		skip "Skipping slow test for local development, test only runs in CI"
	fi

	create_test_environment "$TEST_NAME"

	# create an update
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --add /test_file1

	# start the web server (the server will hang)
	start_web_server -r -H -D "$WEBDIR"
	port=$(get_web_server_port "$TEST_NAME")

	# set the versionurl and contenturl to point to the web server
	set_upstream_server "$TEST_NAME" http://localhost:"$port"/

}

test_teardown() {

	# teardown only if in travis CI
	if [ -n "$TRAVIS" ]; then
		destroy_test_environment "$TEST_NAME"
	fi

}

@test "UPD042: Updating a system, and the upstream server is not responding" {

	# if the upstream server is unresponsive the update should fail after 2 minute
	# timeout with a useful message

	run sudo sh -c "$SWUPD update --allow-insecure-http $SWUPD_OPTS"

	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Update started
		Error: Curl - Communicating with server timed out
		Error: Failed to connect to update server: http://localhost:$port/.*
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \('clrtrust generate' may help\)
		Error: Unable to determine the server version
		Update failed
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test_file1

}

@test "UPD043: Updating a system with a mirror set, and the mirror is not responding" {

	# if the mirror is unresponsive, then the update should fail after 2 minute
	# timeout with a useful message

	# Extra setup: set the mirror in the test environment to point to the web server
	write_to_protected_file "$TARGETDIR"/etc/swupd/mirror_versionurl http://localhost:"$port"/
	write_to_protected_file "$TARGETDIR"/etc/swupd/mirror_contenturl http://localhost:"$port"/

	run sudo sh -c "$SWUPD update --allow-insecure-http $SWUPD_OPTS"

	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Update started
		Checking mirror status
		Error: Curl - Communicating with server timed out
		Error: Failed to connect to update server: http://localhost:$port/.*
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \('clrtrust generate' may help\)
		Warning: Upstream server http://localhost:$port/ not responding, cannot determine upstream version
		Warning: Unable to determine if the mirror is up to date
		Error: Unable to determine the server version
		Update failed
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test_file1

}

@test "UPD044: Updating a system, and the upstream server is not responding but a mirror is set" {

	# if the upstream server is unresponsive but the user has a working
	# mirror set up, then the update should work normally since it should
	# use the mirror

	# Extra setup: set a mirror in the test environment
	create_mirror "$TEST_NAME"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Checking mirror status
		Error: Curl - Communicating with server timed out - 'http://localhost:$port//version/formatstaging/latest'
		Warning: Upstream server http://localhost:$port/ not responding, cannot determine upstream version
		Warning: Unable to determine if the mirror is up to date
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test_file1

}
