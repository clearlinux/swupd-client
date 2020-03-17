#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	# Skip this test for local development because it takes a long time. To run this test locally,
	# configure swupd with --with-fallback-capaths=<path to top level of repo>/swupd_test_certificates
	# and run: RUNNING_IN_CI=true make check
	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	server_pub="$PWD/$TEST_NAME"/server-pub.pem
	server_key="$PWD/$TEST_NAME"/server-key.pem
	client_pub="$PWD/$TEST_NAME"/client-pub.pem
	client_key="$PWD/$TEST_NAME"/client-key.pem

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"

	sudo mkdir -p "$CLIENT_CERT_DIR"

	# create client/server certificates
	generate_certificate "$client_key" "$client_pub"
	generate_certificate "$server_key" "$server_pub"

	# create client certificate in expected directory
	sudo cp "$client_key" "$CLIENT_CERT"
	sudo sh -c "cat $client_pub >> $CLIENT_CERT"

}

global_setup() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	start_web_server -c "$client_pub" -p "$server_pub" -k "$server_key"

}

global_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	destroy_web_server

}

test_setup() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping client certificate tests for local development, test runs in CI"
	fi

	server_pub="$PWD/$TEST_NAME"/server-pub.pem
	server_key="$PWD/$TEST_NAME"/server-key.pem
	client_pub="$PWD/$TEST_NAME"/client-pub.pem
	client_key="$PWD/$TEST_NAME"/client-key.pem

	# trust the web server's public key
	create_trusted_cacert "$server_pub"

	# Set the web server as our upstream server
	port=$(get_web_server_port)
	set_upstream_server "$TEST_NAME" "https://localhost:$port/$TEST_NAME/web-dir"

}

test_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	sudo rm -f "$CLIENT_CERT"
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle
	clean_state_dir "$TEST_NAME"
}

global_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi


	destroy_test_environment "$TEST_NAME"
}

@test "ADD023: Adding a bundle over HTTPS with a valid client certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_exists "$TARGETDIR"/usr/bin/test-file
}

@test "ADD024: Try adding bundle over HTTPS with no client certificate" {

	# remove client certificate
	sudo rm "$CLIENT_CERT"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle --debug"
	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"

	expected_output=$(cat <<-EOM
			.*Curl - Unable to verify server SSL certificate
	EOM
	)
	assert_regex_in_output "$expected_output"
}

@test "ADD025: Try adding bundle over HTTPS with an invalid client certificate" {

	# make client certificate invalid
	sudo sh -c "echo foo > $CLIENT_CERT"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle --debug"
	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"

	expected_output=$(cat <<-EOM
			.*Curl - Problem with the local client SSL certificate
	EOM
	)
	assert_regex_in_output "$expected_output"
}
#WEIGHT=4
