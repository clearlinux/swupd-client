#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	# This test performs dangerous operations, like setting the date
	# It is suppose to fix that later, but as this could fail, only run
	# on protected environments.
	# Note that for this test to work you need to disable any date services
	# like ntp or systemd-timesyncd
	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping slow test for local development, test only runs in CI"
	fi
	if [ -n "${SKIP_DATE_CHECK}" ]; then
		skip "Skipping date test"
	fi

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /file_2

	# Reset date
	DATE=$(date "+%m%d")
	YEAR=$(date "+%Y")
	PREV_YEAR=$((YEAR - 1))
	sudo date "+%Y%m%d" -s "$PREV_YEAR$DATE"

	key_generation_cmd=$(cat <<-EOM
			# Create a root key and cert
			openssl req -x509 -sha512 -days 600 -newkey rsa:4096  -keyout "$TEST_NAME/root.key" -out "$TEST_NAME/root.pem" -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'

			# Create a intermediate key to sign the MoM
			openssl req -sha512 -nodes -newkey rsa:4096 -keyout "$TEST_NAME/cert_mom.key" -out "$TEST_NAME/cert_mom.csr" -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=cert_mom'
			openssl x509 -days 1 -req -in "$TEST_NAME/cert_mom.csr" -CA "$TEST_NAME/root.pem" -CAkey "$TEST_NAME/root.key" -set_serial 0x1 -out "$TEST_NAME/cert_mom.pem"

			# Create a intermediate key to sign the latest
			openssl req -sha512 -nodes -newkey rsa:4096 -keyout "$TEST_NAME/cert_latest.key" -out "$TEST_NAME/cert_latest.csr" -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=cert_latest'
			openssl x509 -days 600 -req -in "$TEST_NAME/cert_latest.csr" -CA "$TEST_NAME/root.pem" -CAkey "$TEST_NAME/root.key" -set_serial 0x1 -out "$TEST_NAME/cert_latest.pem"

			# Sign the MoM with intermediate MoM cert
			openssl smime -sign -binary -in "$WEB_DIR"/10/Manifest.MoM -signer "$TEST_NAME/cert_mom.pem" -inkey "$TEST_NAME/cert_mom.key" -out "$WEB_DIR"/10/Manifest.MoM.sig -outform DER
			openssl smime -sign -binary -in "$WEB_DIR"/20/Manifest.MoM -signer "$TEST_NAME/cert_mom.pem" -inkey "$TEST_NAME/cert_mom.key" -out "$WEB_DIR"/20/Manifest.MoM.sig -outform DER

			# Sign the latest with cert_latest latest cert
			openssl smime -sign -binary -in "$WEB_DIR"/version/formatstaging/latest -signer "$TEST_NAME/cert_latest.pem" -inkey "$TEST_NAME/cert_latest.key" -out "$WEB_DIR"/version/formatstaging/latest.sig -outform DER

	EOM
	)
	sudo sh -c "$key_generation_cmd"

	# Restore date
	sudo date "+%Y%m%d" -s "$YEAR$DATE"

}

@test "SIG027: Swupd update with expired MoM certificate" {

	# Swupd should fail if MoM certificate is expired even if latest certificate
	# is not expired.
	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_CERT -C $TEST_NAME/root.pem"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Warning: Signature check failed
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Warning: Signature check failed
		Error: Signature verification failed for manifest version 10
		Update failed
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/file_2

}
