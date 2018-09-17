#!/usr/bin/env bats

load "../testlib"

server_pub="/tmp/$TEST_NAME-server-pub.pem"
server_key="/tmp/$TEST_NAME-server-key.pem"
client_pub="/tmp/$TEST_NAME-client-pub.pem"
client_key="/tmp/$TEST_NAME-client-key.pem"

global_setup() {

	# Skip this test for local development because it takes a long time. To run this
	# test locally, configure swupd with --fallback-ca-paths=/tmp/swup_test_certificates
	# and run: TRAVIS=true make check
	if [ -z "${TRAVIS}" ]; then
		return
	fi

	create_test_environment "$TEST_NAME"
	create_test_environment "$TEST_NAME" 100

	sudo mkdir -p "$CLIENT_CERT_DIR"

	# create client/server certificates
	generate_certificate "$client_key" "$client_pub"
	generate_certificate "$server_key" "$server_pub"

	# add server pub key to trust store
	create_trusted_cacert "$server_pub"

	start_web_server -c "$client_pub" -p "$server_pub" -k "$server_key"
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

	destroy_web_server
	destroy_trusted_cacert

	rm -f "$client_key"
	rm -f "$client_pub"
	rm -f "$server_key"
	rm -f "$server_pub"

	destroy_test_environment "$TEST_NAME"
}

@test "check update with valid client cert" {

	opts="-S $TEST_DIRNAME/state -p $TEST_DIRNAME/target-dir -F staging -u https://localhost:$PORT/$TEST_NAME/web-dir"

	run sudo sh -c "$SWUPD check-update $opts"

	assert_status_is 0
}

@test "check update with no client cert" {

	opts="-S $TEST_DIRNAME/state -p $TEST_DIRNAME/target-dir -F staging -u https://localhost:$PORT/$TEST_NAME/web-dir"

	# remove client certificate
	sudo rm "$CLIENT_CERT"

	run sudo sh -c "$SWUPD check-update $opts"
	assert_status_is "$ENOSWUPDSERVER"

	expected_output=$(cat <<-EOM
			Error: unable to verify server SSL certificate
	EOM
	)
	assert_in_output "$expected_output"
}

@test "check update with invalid client cert" {

	opts="-S $TEST_DIRNAME/state -p $TEST_DIRNAME/target-dir -F staging -u https://localhost:$PORT/$TEST_NAME/web-dir"

	# make client certificate invalid
	sudo sh -c "echo foo > $CLIENT_CERT"

	run sudo sh -c "$SWUPD check-update $opts"
	assert_status_is "$ENOSWUPDSERVER"

	expected_output=$(cat <<-EOM
			Curl: Problem with the local client SSL certificate
	EOM
	)
	assert_in_output "$expected_output"
}
