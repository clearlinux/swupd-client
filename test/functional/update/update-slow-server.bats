#!/usr/bin/env bats

load "../testlib"

server_pid=""
port=""
THEME_DIRNAME="$FUNC_DIR/update"

global_setup() {

	# Skip this test if not running in Travis CI, because test takes too long for
	# local development. To run this locally do: TRAVIS=true make check
	if [ -z "${TRAVIS}" ]; then
		skip "Skipping slow test for local development, test only runs in CI"
	fi
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/bar "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" test-bundle --update /foo/bar

	start_web_server -d pack-test-bundle-from-10.tar -s
}

test_setup() {
	return
}

test_teardown() {
	return
}

global_teardown() {

	# teardown only if in travis CI
	if [ -n "${TRAVIS}" ]; then
		destroy_web_server
		destroy_test_environment "$TEST_NAME"
	fi

}

@test "update --download with a slow server" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_HTTP"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Error for .*/state/pack-test-bundle-from-10-to-100.tar download: Response 206 - No error
		Starting download retry #1 for .*100/pack-test-bundle-from-10.tar
		Error for .*/state/pack-test-bundle-from-10-to-100.tar download: Response 200 - Requested range was not delivered by the server
		Range command not supported by server, download resume disabled.
		Starting download retry #2 for .*100/pack-test-bundle-from-10.tar
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
