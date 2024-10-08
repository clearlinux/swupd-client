#!/usr/bin/env bats

load "../testlib"

server_pub="$PWD/$TEST_NAME"/server-pub.pem
server_key="$PWD/$TEST_NAME"/server-key.pem
client_pub="$PWD/$TEST_NAME"/client-pub.pem
client_key="$PWD/$TEST_NAME"/client-key.pem

setup_file() {

	# Skip this test for local development because it takes a long time. To run this test locally,
	# configure swupd with --with-fallback-capaths=<path to top level of repo>/swupd_test_certificates
	# and run: RUNNING_IN_CI=true make check
	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10

	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"

	sudo mkdir -p "$ABS_CLIENT_CERT_DIR"

	# create client/server certificates
	generate_certificate "$client_key" "$client_pub"
	generate_certificate "$server_key" "$server_pub"

	# add server pub key to trust store
	create_trusted_cacert "$server_pub"

	start_web_server -c "$client_pub" -p "$server_pub" -k "$server_key"

	# Set the web server as our upstream server
	port=$(get_web_server_port "$TEST_NAME")
	set_upstream_server "$TEST_NAME" "https://localhost:$port/$TEST_NAME/web-dir"

}

teardown_file() {

	# Skip this test for local development because it takes a long time. To run this test locally,
	# configure swupd with --with-fallback-capaths=<path to top level of repo>/swupd_test_certificates
	# and run: RUNNING_IN_CI=true make check
	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	destroy_test_environment --force "$TEST_NAME"

}

test_setup() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping client certificate tests for local development, test runs in CI"
	fi

	set_current_version "$TEST_NAME" 10

	# create client certificate in expected directory
	sudo cp "$client_key" "$CLIENT_CERT_FILE"
	sudo sh -c "cat $client_pub >> $CLIENT_CERT_FILE"
}

test_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	sudo rm -f "$CLIENT_CERT_FILE"
	clean_state_dir "$TEST_NAME"
}

@test "CHK006: Check for available updates over HTTPS with a valid client certificate" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is 0
}

@test "CHK007: Try checking for available updates over HTTPS with no client certificate" {

	# remove client certificate
	sudo rm "$CLIENT_CERT_FILE"

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS --debug"
	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"

	expected_output=$(cat <<-EOM
			.*Curl - Unable to verify server SSL certificate
	EOM
	)
	assert_regex_in_output "$expected_output"
}

@test "CHK008: Try checking for available updates over HTTPS with an invalid client certificate" {

	# make client certificate invalid
	sudo sh -c "echo foo > $CLIENT_CERT_FILE"

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS --debug"
	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"

	expected_output=$(cat <<-EOM
			.*Curl - Problem with the local client SSL certificate
	EOM
	)
	assert_regex_in_output "$expected_output"
}
#WEIGHT=4
