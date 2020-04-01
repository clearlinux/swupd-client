#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

export repo1
export repo2

test_setup() {
	# Skip this test for local development because we write the certificate on /
    # This is necessary because the default certificate location is hardcoded on build time and we
	# need to run swupd without -C parameter to test this feature.
	if [ -z "${RUNNING_IN_CI}" ]; then
		skip "Skipping test because it will make changes to your system"
	fi

	create_test_environment "$TEST_NAME"

	if [ ! -f /usr/share/clear/update-ca/Swupd_Root.pem ]; then
		sudo mkdir -p /usr/share/clear/update-ca
		sudo cp "$TEST_DIRNAME"/Swupd_Root.pem /usr/share/clear/update-ca
		export CERT_WAS_INSTALLED=1
	fi

	create_third_party_repo "$TEST_NAME" 10 staging test-repo1
	repo1="$TPURL"
	create_third_party_repo "$TEST_NAME" 10 staging test-repo2
	repo2="$TPURL"
}

test_teardown() {

	if [ -z "${RUNNING_IN_CI}" ]; then
		return
	fi

	if [ -n "${CERT_WAS_INSTALLED}" ]; then
		sudo rm usr/share/clear/update-ca/Swupd_Root.pem
	fi
}

@test "TPR066: Add a single repo importing the certificate" {

	run sudo sh -c "echo 'y' | $SWUPD 3rd-party add test-repo1 file://$repo1 $SWUPD_OPTS_NO_CERT"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Importing 3rd-party repository public certificate:
		Issuer: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		Subject: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		.*
		Adding 3rd-party repository test-repo1...
		Installing the required bundle 'os-core' from the repository...
		Note that all bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Successfully installed 1 bundle
		Repository added successfully
	EOM
	)
	assert_regex_is_output "$expected_output"

	run sudo sh -c "cat $PATH_PREFIX/$THIRD_PARTY_DIR/repo.ini"
	expected_output=$(cat <<-EOM
			[test-repo1]
			url=file://$repo1
	EOM
	)
	assert_is_output "$expected_output"

	# make sure the os-core bundle of the repo is installed in the appropriate place
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/lib/os-release

}

@test "TPR067: Reject a certificate when adding a repository" {

	run sudo sh -c "echo 'n' | $SWUPD 3rd-party add test-repo1 file://$repo1 $SWUPD_OPTS_NO_CERT"
	assert_status_is "$SWUPD_BAD_CERT"
	expected_output=$(cat <<-EOM
		Importing 3rd-party repository public certificate:
		Issuer: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		Subject: /C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost
		.*
		Failed to add repository
	EOM
	)
	assert_regex_is_output "$expected_output"
}
