#!/usr/bin/env bats

load "../testlib"

server_pid=""
port=""
THEME_DIRNAME="$FUNC_DIR/checkupdate_v2"

test_teardown() {

	echo "terminating web server..." >&3
	kill "$server_pid" >&3
	destroy_test_environment "$TEST_NAME"

}

@test "check-update with a slow server" {

	# Pre-req: create a web server that can serve as a slow content download server
	echo "starting web server..." >&3
	start_web_server
	slow_opts="-p $TEST_NAME/target-dir -F staging -u http://localhost:$port/"

	# test
	run sudo sh -c "$SWUPD check-update $slow_opts"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Current OS version: 10
		There is a new OS version available: 99990
	EOM
	)
	assert_in_output "$expected_output"

}

start_web_server() {

	for i in {8080..8180}; do
		"$THEME_DIRNAME"/server.py $i &
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
			echo "responding"
			break
		fi
	done

}