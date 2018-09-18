#!/usr/bin/env bats

load "../testlib"

server_pid=""
port=""
THEME_DIRNAME="$FUNC_DIR/update"

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

}

test_teardown() {

	# teardown only if in travis CI
	if [ -n "${TRAVIS}" ]; then
		print "terminating web server (PID: $server_pid)..."
		kill "$server_pid" >&3
		print "server killed"
		destroy_test_environment "$TEST_NAME"
	fi

}

@test "update --download with a slow server" {

	# Pre-req: create a web server that can serve as a slow content download server
	print "starting web server..."
	start_web_server
	slow_opts="-u http://localhost:$port/"

	run sudo sh -c "$SWUPD update $slow_opts $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Error for .*update-slow-server/state/pack-test-bundle-from-10-to-100.tar download: Response 206 - No error
		Starting download retry #1 for .*100/pack-test-bundle-from-10.tar
		Error for .*update-slow-server/state/pack-test-bundle-from-10-to-100.tar download: Response 200 - Requested range was not delivered by the server
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

start_web_server() {

	for i in {8081..8181}; do
		"$THEME_DIRNAME"/update_slow_server.py $i &
		sleep .5
		server_pid=$!
		if [ -d /proc/$server_pid ]; then
			port=$i
			break
		fi
	done

	# wait until server becomes available by expecting a successful curl
	for i in $(seq 1 10); do
		flag=true
		curl http://localhost:"$port"/ || flag=false
		if [ "$flag" == false ]; then
			sleep .5
			continue
		else
			print "server up"
			break
		fi
	done

}
