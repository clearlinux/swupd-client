#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10

}

@test "UPD009: Try updating a system where the 'from' MoM is not found" {

	# remove the "from" mom
	sudo rm -rf "$WEBDIR"/10/Manifest.MoM.tar

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Failed to retrieve 10 MoM manifest
		Update failed
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "UPD010: Try updating a system where the 'to' MoM is not found" {

	# remove the "to" mom
	sudo rm -rf "$WEBDIR"/20/Manifest.MoM.tar

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Error: Failed to retrieve 20 MoM manifest
		Update failed
	EOM
	)
	assert_regex_is_output "$expected_output"

}