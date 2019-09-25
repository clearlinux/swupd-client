#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	export CERT_PATH="/usr/share/clear/update-ca/Swupd_Root.pem"
	export SWUPD_OPTS_EXTRA="$SWUPD_OPTS_NO_FMT_NO_CERT -C $TARGETDIR$CERT_PATH"

	create_version -r "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --add "$CERT_PATH":"$TEST_ROOT_DIR"/Swupd_Root.pem
	bump_format "$TEST_NAME"

	# Rotate the key inside the content. We need to sign release 30 using old root key and sign the release 40 and 50 using the new key. And include the new key in releases 30, 40 and 50.
	generate_certificate "$TEST_NAME/new_root.key" "$TEST_NAME/new_root.pem"
	sudo sh -c "mv $WEBDIR/40 $WEBDIR/000"
	sudo sh -c "sed -i '/Swupd_Root.pem/d' $WEBDIR/30/Manifest.os-core"

	update_bundle "$TEST_NAME" os-core --add "$CERT_PATH":"$TEST_NAME"/new_root.pem

	sudo sh -c "mv $WEBDIR/000 $WEBDIR/40"
	sudo sh -c "sed -i '/Swupd_Root.pem/d' $WEBDIR/40/Manifest.os-core"
	sudo sh -c "grep 'Swupd_Root.pem' $WEBDIR/30/Manifest.os-core >> $WEBDIR/40/Manifest.os-core"
	sudo sh -c "sed -i 's/\t30\t/\t40\t/' $WEBDIR/40/Manifest.os-core"

	create_version -r "$TEST_NAME" 50 40 2

	# Create the key to rotate and re-sign content after the bump
	sign_cmd=$(cat <<-EOM
			# Sign the MoM with self signed intermediate cert
			openssl smime -sign -binary -in "$WEBDIR"/40/Manifest.MoM -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEBDIR"/40/Manifest.MoM.sig -outform DER
			openssl smime -sign -binary -in "$WEBDIR"/50/Manifest.MoM -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEBDIR"/50/Manifest.MoM.sig -outform DER
	EOM
	)

	sudo sh -c "$sign_cmd"

}

@test "SIG017: Key rotation" {

        sudo openssl smime -sign -binary -in "$WEBDIR"/version/latest_version -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEBDIR"/version/latest_version.sig -outform DER

        sudo openssl smime -sign -binary -in "$WEBDIR"/version/format2/latest -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEBDIR"/version/format2/latest.sig -outform DER

	run sudo sh -c "$SWUPD update -V 20 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$SWUPD_OK"

	assert_file_exists "$TARGETDIR""$CERT_PATH"
	run sudo sh -c "$SWUPD update -V 30 $SWUPD_OPTS_EXTRA"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 20 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 0
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

	run sudo sh -c "$SWUPD update -V 50 $SWUPD_OPTS_EXTRA"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 40 to 50
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 40 to version 50:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 40 to version 50
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

}

@test "SIG018: Using incorrect keys should fail after key rotation" {

	run sudo sh -c "$SWUPD update -V 20 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR""$CERT_PATH"

	run sudo sh -c "$SWUPD update -V 30 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 20 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 0
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

	run sudo sh -c "$SWUPD update -V 50 $SWUPD_OPTS_NO_FMT"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 40 to 50
		Error: Certificate verification error - self signed certificate
		Error: Signature check error
		Signature check failed!
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

}
