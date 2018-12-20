#!/usr/bin/env bats

load "../testlib"

test_setup() {

	# Skip this test if not running in Travis CI, because test takes too long for
	# local development. To run this locally do: TRAVIS=true make check
	if [ -z "${TRAVIS}" ]; then
		skip "Skipping slow test for local development, test only runs in CI"
	fi
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/bar "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --update /foo/bar

	start_web_server -D "$WEBDIR" -d 100/pack-test-bundle-from-10.tar -s

	# Set the web server as our upstream server
	port=$(get_web_server_port "$TEST_NAME")
	set_upstream_server "$TEST_NAME" "http://localhost:$port"

}

test_teardown() {

	# teardown only if in travis CI
	if [ -n "${TRAVIS}" ]; then
		destroy_test_environment "$TEST_NAME"
	fi

}

@test "UPD025: Updating a system using a slow server" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Curl: File incompletely downloaded - '.*/100/pack-test-bundle-from-10.tar'
		Starting download retry #1 for .*/100/pack-test-bundle-from-10.tar
		Curl: Resuming download for '.*/100/pack-test-bundle-from-10.tar'
		Range command not supported by server, download resume disabled - '.*/100/pack-test-bundle-from-10.tar'
		Starting download retry #2 for .*/100/pack-test-bundle-from-10.tar
		Extracting test-bundle pack for version 100
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/bar

}
