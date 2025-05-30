#!/usr/bin/env bats

load "../testlib"

test_setup() {

	# Skip this test if not running in Travis CI, because test takes too long for
	# local development. To run this locally do: RUNNING_IN_CI=true make check
	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping slow test for local development, test only runs in CI"
	fi
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/bar "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --update /foo/bar

	start_web_server -D "$WEB_DIR" -d 100/pack-test-bundle-from-10.tar -s

	# Set the web server as our upstream server
	port=$(get_web_server_port "$TEST_NAME")
	set_upstream_server "$TEST_NAME" "http://localhost:$port"

}

@test "UPD025: Updating a system using a slow server" {

	run sudo sh -c "$SWUPD update --allow-insecure-http $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Update started
		Preparing to update from 10 to 100 \(in format staging\)
		Downloading packs for:
		 - test-bundle
		Error: Curl - File incompletely downloaded - '.*/100/pack-test-bundle-from-10.tar'
		Waiting 10 seconds before retrying the download
		Retry #1 downloading from .*/100/pack-test-bundle-from-10.tar
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/foo/bar

}
#WEIGHT=40
