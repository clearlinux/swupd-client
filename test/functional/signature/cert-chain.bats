#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

	key_generation_cmd=$(cat <<-EOM
			# Create a root key and cert
			openssl req -x509 -sha512 -days 1 -newkey rsa:4096  -keyout "$TEST_NAME/root.key" -out "$TEST_NAME/root.pem" -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'
			# Create a intermediate key and cert signed by root
			openssl req -sha512 -nodes -newkey rsa:4096 -keyout "$TEST_NAME/intermediate.key" -out "$TEST_NAME/intermediate.csr" -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=intermediate'
			openssl x509 -days 1 -req -in "$TEST_NAME/intermediate.csr" -CA "$TEST_NAME/root.pem" -CAkey "$TEST_NAME/root.key" -set_serial 0x1 -out "$TEST_NAME/intermediate.pem"
			# Create a self signed certificate for intermediate
			openssl req -days 1 -new -x509 -key "$TEST_NAME/intermediate.key" -out "$TEST_NAME/intermediate_self_signed.pem" -set_serial 0x1 -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=intermediate'
			# Sign the MoM with intermediate cert
			openssl smime -sign -binary -in "$WEBDIR"/10/Manifest.MoM -signer "$TEST_NAME/intermediate.pem" -inkey "$TEST_NAME/intermediate.key" -out "$WEBDIR"/10/Manifest.MoM.sig -outform DER
			# Sign the MoM with self signed intermediate cert
			openssl smime -sign -binary -in "$WEBDIR"/10/Manifest.MoM -signer "$TEST_NAME/intermediate_self_signed.pem" -inkey "$TEST_NAME/intermediate.key" -out "$WEBDIR"/10/Manifest.MoM.sig_self_signed -outform DER
			# Create a certificate store with both root and self signed intermediate
			cat "$TEST_NAME/root.pem" > "$TEST_NAME/trust.pem"
			cat "$TEST_NAME/intermediate_self_signed.pem" >> "$TEST_NAME/trust.pem"
	EOM
	)

	sudo sh -c "$key_generation_cmd"

}

@test "SIG010: MoM signed with intermediate key should fail" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: FAILED TO VERIFY SIGNATURE OF Manifest.MoM version 10!!!
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG011: MoM signed with intermediate key and root cert is used" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/root.pem test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}

@test "SIG012: MoM signed with self-signed cert, but using root to validate" {

	# replace sig file to one signed with intermediate_self_signed.pem
	sudo sh -c "cp $WEBDIR/10/Manifest.MoM.sig_self_signed $WEBDIR/10/Manifest.MoM.sig"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/root.pem test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: FAILED TO VERIFY SIGNATURE OF Manifest.MoM version 10!!!
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG013: MoM signed with self_signed and using correct cert to validate" {

	# replace sig file to one signed with intermediate_self_signed.pem
	sudo sh -c "cp $WEBDIR/10/Manifest.MoM.sig_self_signed $WEBDIR/10/Manifest.MoM.sig"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/intermediate_self_signed.pem test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}

@test "SIG014: MoM signed with intermediate, but self signed intermediate is used to validate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/intermediate_self_signed.pem test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: FAILED TO VERIFY SIGNATURE OF Manifest.MoM version 10!!!
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG015: MoM signed with intermediate and trust store is used" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/trust.pem test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}

@test "SIG016: MoM is signed with self signed certificate and trust store is used" {

	# replace signature with self signed certificate
	sudo sh -c "cp $WEBDIR/10/Manifest.MoM.sig_self_signed $WEBDIR/10/Manifest.MoM.sig"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/trust.pem test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
