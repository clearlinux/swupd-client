#!/usr/bin/env bats

load "../testlib"

server_pub="/tmp/$TEST_NAME-server-pub.pem"
server_key="/tmp/$TEST_NAME-server-key.pem"
client_pub="/tmp/$TEST_NAME-client-pub.pem"
client_key="/tmp/$TEST_NAME-client-key.pem"

global_setup() {

	# Skip this test for local development because it takes a long time. To run this
	# test locally, configure swupd with --with-fallback-capaths=/tmp/swup_test_certificates
	# and run: TRAVIS=true make check
	if [ -z "${TRAVIS}" ]; then
		return
	fi

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /foo/test-file "$TEST_NAME"

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

	install_bundle "$TEST_NAME"/web-dir/10/Manifest.test-bundle

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

@test "remove bundle with valid client cert" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS_HTTPS test-bundle"

	assert_status_is 0
	assert_file_not_exists "$TEST_NAME"/target-dir/foo/test-file
}

@test "remove bundle with no client cert" {

	# remove client certificate
	sudo rm "$CLIENT_CERT"

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS_HTTPS test-bundle"
	assert_status_is "$ECURL_INIT"

	expected_output=$(cat <<-EOM
			Error: unable to verify server SSL certificate
	EOM
	)
	assert_in_output "$expected_output"
}

@test "remove bundle with invalid client cert" {

	# make client certificate invalid
	sudo sh -c "echo foo > $CLIENT_CERT"

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS_HTTPS test-bundle"
	assert_status_is "$ECURL_INIT"

	expected_output=$(cat <<-EOM
			Curl: Problem with the local client SSL certificate
	EOM
	)
	assert_in_output "$expected_output"
}
