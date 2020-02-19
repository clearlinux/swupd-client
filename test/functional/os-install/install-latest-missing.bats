#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove the formatstaging/latest
	sudo rm -rf "$WEBDIR/version/formatstaging/latest"

}

@test "INS012: Try doing an OS install based on the latest version and a version file can't be found on server" {

	# if swupd cannot determine what the "latest" version is, it should fail

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --version latest"

	assert_status_is_not "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Error: Unable to get latest version for install
		Installation failed
	EOM
	)
	assert_in_output "$expected_output"

}
#WEIGHT=1
