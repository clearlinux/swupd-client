#!/usr/bin/env bats

# Author: William Douglas
# Email: william.douglas@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	export CERT_PATH="/usr/share/clear/update-ca/Swupd_Root.pem"
	export ALT_CERT_PATH="$CERT_PATH.alt"
	export SWUPD_OPTS_EXTRA="$SWUPD_OPTS_NO_FMT_NO_CERT -C $TARGET_DIR$CERT_PATH"

	create_version -r "$TEST_NAME" 20 10
	generate_certificate "$TEST_NAME/new_root.key" "$TEST_NAME/new_root.pem"
	update_bundle -p "$TEST_NAME" os-core --add "$CERT_PATH":"$ABS_TEST_DIR"/Swupd_Root.pem
	update_bundle "$TEST_NAME" os-core --add "$ALT_CERT_PATH":"$TEST_NAME/new_root.pem"
	bump_format "$TEST_NAME"

	create_version -r "$TEST_NAME" 50 40 2

}

@test "SIG029: alt key usable" {

	# Test the alternate keyfile is usable

	# file only used in check-update so not needed here
	# sudo openssl smime -sign -binary -in "$WEB_DIR"/version/latest_version -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEB_DIR"/version/latest_version.sig -outform DER

	# use the new cert to force the alt cert into use for the first time
	sudo openssl smime -sign -binary -in "$WEB_DIR"/version/format2/latest -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEB_DIR"/version/format2/latest.sig -outform DER

	# Sign the MoM with self signed intermediate cert
	# This is verified first, use old cert to test swapping from alt to original certs
	# sudo openssl smime -sign -binary -in "$WEB_DIR"/40/Manifest.MoM -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEB_DIR"/40/Manifest.MoM.sig -outform DER

	# verified second, use new cert to test two cert swaps are usable in the same update
	# and different manifest versions can be verified with different certs
	sudo openssl smime -sign -binary -in "$WEB_DIR"/50/Manifest.MoM -signer "$TEST_NAME/new_root.pem" -inkey "$TEST_NAME/new_root.key" -out "$WEB_DIR"/50/Manifest.MoM.sig -outform DER

	run sudo sh -c "$SWUPD update -V 20 $SWUPD_OPTS_NO_FMT"
	assert_status_is "$SWUPD_OK"

	assert_file_exists "$TARGET_DIR""$CERT_PATH"
	assert_file_exists "$TARGET_DIR""$ALT_CERT_PATH"
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
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/core

	run sudo sh -c "$SWUPD update -V 50 $SWUPD_OPTS_EXTRA"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Warning: Default cert failed, attempting to use alternative: .*alt
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
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 40 to version 50
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/core

}
