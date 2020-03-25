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
	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

	# Resign release with an expired key
	sudo rm "$WEBDIR"/version/latest_version.sig
	sudo rm "$WEBDIR"/version/formatstaging/latest.sig
	sudo rm "$WEBDIR"/10/Manifest.MoM.sig

	sudo systemctl stop systemd-timesyncd
	DATE=$(date "+%Y%m%d %H:%M:%S")
	sudo date "+%Y%m%d" -s "20000101" #go back to the past
	sudo sh -c "openssl req -x509 -sha512 -days 1 -newkey rsa:4096  -keyout $TEST_NAME/root.key -out $TEST_NAME/root.pem -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'"
	sudo sh -c "openssl smime -sign -binary -in $WEBDIR/10/Manifest.MoM -signer $TEST_NAME/root.pem -inkey $TEST_NAME/root.key -out $WEBDIR/10/Manifest.MoM.sig -outform DER"
	sudo date "+%Y%m%d %H:%M:%S" -s "$DATE"
	sudo systemctl start systemd-timesyncd

}

@test "SIG025: Swupd bundle-add with expired certificate" {

	# Swupd shouldn't accept expired certificate for MoM on any operation

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/root.pem test-bundle"

	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"
	expected_output=$(cat <<-EOM
		Error: Failed to verify certificate: certificate has expired
		Error: Failed swupd initialization, exiting now
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}
#WEIGHT=4
