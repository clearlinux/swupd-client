#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

	# Resign release
	sudo rm "$WEBDIR"/version/latest_version.sig
	sudo rm "$WEBDIR"/version/formatstaging/latest.sig
	sudo rm "$WEBDIR"/10/Manifest.MoM.sig

	sudo sh -c "openssl req -x509 -sha512 -days 1 -newkey rsa:4096  -keyout $TEST_NAME/root.key -out $TEST_NAME/root.pem -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'"
	sudo sh -c "openssl smime -sign -binary -in $WEBDIR/10/Manifest.MoM -signer $TEST_NAME/root.pem -inkey $TEST_NAME/root.key -out $WEBDIR/10/Manifest.MoM.sig -outform DER"

}

@test "SIG024: Swupd bundle-add with valid certificate" {

	# Operations should work when using a custom valid certificate

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/root.pem test-bundle"

	assert_status_is "$SWUPD_OK"

}
#WEIGHT=2
