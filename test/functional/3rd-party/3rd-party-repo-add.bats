#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

export repo1
export repo2

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo "$TEST_NAME" 10 staging test-repo1
	repo1="$TPWEBDIR"
	create_third_party_repo "$TEST_NAME" 10 staging test-repo2
	repo2="$TPWEBDIR"

}

@test "TPR001: Add a single repo" {

	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
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
	assert_is_output "$expected_output"

	run sudo sh -c "cat $STATEDIR/3rd_party/repo.ini"
	expected_output=$(cat <<-EOM
			[test-repo1]
			url=file://$repo1
	EOM
	)
	assert_is_output "$expected_output"

	# make sure the os-core bundle of the repo is installed in the appropriate place
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo1/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo1/usr/lib/os-release

}

@test "TPR002: Add multiple repos in repo config" {

	# Add multiple repos in a row one by one
	run sudo sh -c "$SWUPD 3rd-party add test-repo1 file://$repo1 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository added successfully
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo1/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo1/usr/lib/os-release

	run sudo sh -c "$SWUPD 3rd-party add test-repo2 file://$repo2 $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			Repository added successfully
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo2/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/opt/3rd_party/test-repo2/usr/lib/os-release

	run sudo sh -c "cat $STATEDIR/3rd_party/repo.ini"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
			[test-repo1]
			url=file://$repo1

			[test-repo2]
			url=file://$repo2
	EOM
	)
	assert_is_output --identical "$expected_output"

}
