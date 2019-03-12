#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"
	sudo rm "$WEBDIR"/10/Manifest.MoM.sig

}

@test "ADD026: Try adding a bundle without a MoM signature" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Warning: FAILED TO VERIFY SIGNATURE OF Manifest.MoM version 10!!!
		Cannot load official manifest MoM for version 10
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "ADD027: Force adding a bundle without a MoM signature" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS --nosigcheck test-bundle"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		FAILED TO VERIFY SIGNATURE OF Manifest.MoM. Operation proceeding due to
		  --nosigcheck, but system security may be compromised
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
