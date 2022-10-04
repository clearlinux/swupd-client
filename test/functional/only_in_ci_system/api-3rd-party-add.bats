#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# Skip this test for local development because we write the certificate on /
	# This is necessary because the default certificate location is hardcoded on build time and we
	# need to run swupd without -C parameter to test this feature.
	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping test because it will make changes to your system"
	fi

	create_test_environment "$TEST_NAME"

	sudo rm -f /usr/share/clear/update-ca/Swupd_Root.pem
	sudo mkdir -p /usr/share/clear/update-ca
	sudo cp test/functional/only_in_ci_system/pemfile /usr/share/clear/update-ca/Swupd_Root.pem

	create_third_party_repo "$TEST_NAME" 10 staging repo1
	export repo1="$ABS_TP_URL"

}

test_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	sudo rm -f /usr/share/clear/update-ca/Swupd_Root.pem

}

@test "API069: 3rd-party add" {

	run sudo sh -c "echo 'y' | $SWUPD 3rd-party add $SWUPD_OPTS_NO_CERT my-repo file://$repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Issuer: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		Subject: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		-----BEGIN CERTIFICATE-----
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		-----END CERTIFICATE-----
		To add the 3rd-party repository you need to accept this certificate
		Do you want to continue? (y/N):
	EOM
	)
	assert_in_output "$expected_output"

}

@test "API070: 3rd-party add --assume=yes" {

	run sudo sh -c "$SWUPD 3rd-party add $SWUPD_OPTS_NO_CERT my-repo file://$repo1 --assume=yes --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Issuer: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		Subject: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		-----BEGIN CERTIFICATE-----
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		-----END CERTIFICATE-----
		To add the 3rd-party repository you need to accept this certificate
		Do you want to continue? (y/N):
	EOM
	)
	assert_in_output "$expected_output"

}
