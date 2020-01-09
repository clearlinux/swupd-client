#!/usr/bin/env bats

load "../testlib"

server1_pub="$PWD/$TEST_NAME"/server1-pub.pem
server2_pub="$PWD/$TEST_NAME"/server2-pub.pem
server1_key="$PWD/$TEST_NAME"/server1-key.pem
server2_key="$PWD/$TEST_NAME"/server2-key.pem
client_pub="$PWD/$TEST_NAME"/client-pub.pem
client_key="$PWD/$TEST_NAME"/client-key.pem
server1_port=38000
server2_port=42000
server1_hostname="localhost1"
server2_hostname="localhost2"

global_setup() {

	# Skip this test for local development because it takes a long time. To run this test locally,
	# configure swupd with --with-fallback-capaths=<path to top level of repo>/swupd_test_certificates
	# and run: TRAVIS=true make check
	if [ -z "${TRAVIS}" ]; then
		return
	fi

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10

	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"

	sudo mkdir -p "$CLIENT_CERT_DIR"

	# create client/server certificates
	generate_certificate "$client_key" "$client_pub"

	# server 1
	generate_certificate "$server1_key" "$server1_pub"

	# server 2
	generate_certificate "$server2_key" "$server2_pub"

	# add server(1&2) pub key to trust store
	create_trusted_cacert "$server1_pub"
	create_trusted_cacert "$server2_pub"

	# Starting server1
	start_web_server -c "$client_pub" -p "$server1_pub" -k "$server1_key" -P "$server1_port" -h "$server1_hostname"

	# Starting server2
	start_web_server -c "$client_pub" -p "$server2_pub" -k "$server2_key" -P "$server2_port" -h "$server2_hostname"

	# Set the server1 as the default upstream server
	set_upstream_server "$TEST_NAME" "https://$server1_hostname:$server1_port/$TEST_NAME/web-dir"

}

test_setup() {

	if [ -z "${TRAVIS}" ]; then
		skip "Skipping client certificate tests for local development, test runs in CI"
	fi

	set_current_version "$TEST_NAME" 10

	# create client certificate in expected directory
	sudo cp "$client_key" "$CLIENT_CERT"
	sudo sh -c "cat $client_pub >> $CLIENT_CERT"
}

test_teardown() {

	if [ -z "${TRAVIS}" ]; then
		return
	fi

	sudo rm -f "$CLIENT_CERT"
	clean_state_dir "$TEST_NAME"
}

global_teardown() {

	if [ -z "${TRAVIS}" ]; then
		return
	fi


	destroy_test_environment "$TEST_NAME"
}

@test "CHK013: Check for available updates over HTTPS with different URL" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS -u https://$server2_hostname:$server2_port/$TEST_NAME/web-dir"

	assert_status_is "$SWUPD_OK"
}
