#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	add_third_party_repo "$TEST_NAME" 10 1 repo1

	# create a file with the setuid flag set
	file_2=$(create_file -u "$TPWEBDIR"/10/files)

	# create a file with the setgid flag set
	file_3=$(create_file -g "$TPWEBDIR"/10/files)

	# create a bundle that has the file previosuly created
	create_bundle -n test-bundle1 -f /foo/file_1,/bar/file_2:"$file_2",/bar/file_3:"$file_3" -u repo1 "$TEST_NAME"

}

@test "TPR060: Trying to add a bundle that has dangerous flags from a 3rd-party repository - confirm installation" {

	# If a bundle from a 3rd-party repository has the setuid, setgid or sticky bit flags
	# set, then it needs to use the --force flag to be installed since those flags are
	# dangerous

	run sudo sh -c "echo 'y' | $SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Validating 3rd-party bundle file permissions...
		Warning: File /bar/file_2 has dangerous permissions
		Warning: File /bar/file_3 has dangerous permissions
		Warning: The 3rd-party bundle you are about to install contains files with dangerous permission
		Do you want to continue? (y/N):$SPACE
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR061: Trying to add a bundle that has dangerous flags from a 3rd-party repository - cancell installation" {

	# If a bundle from a 3rd-party repository has the setuid, setgid or sticky bit flags
	# set, then it needs to use the --force flag to be installed since those flags are
	# dangerous

	run sudo sh -c "echo 'n' | $SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_INVALID_FILE"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Validating 3rd-party bundle file permissions...
		Warning: File /bar/file_2 has dangerous permissions
		Warning: File /bar/file_3 has dangerous permissions
		Warning: The 3rd-party bundle you are about to install contains files with dangerous permission
		Do you want to continue? (y/N):$SPACE
		Aborting bundle installation...
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR065: Trying to add a bundle that has dangerous flags from a 3rd-party repository using --assume to run non interactively" {

	# If a bundle from a 3rd-party repository has the setuid, setgid or sticky bit flags
	# set, then it needs to use the --force flag to be installed since those flags are
	# dangerous

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1 --assume=no"

	assert_status_is "$SWUPD_INVALID_FILE"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Validating 3rd-party bundle file permissions...
		Warning: File /bar/file_2 has dangerous permissions
		Warning: File /bar/file_3 has dangerous permissions
		Warning: The 3rd-party bundle you are about to install contains files with dangerous permission
		Do you want to continue? (y/N): N
		The "--assume=no" option was used
		Aborting bundle installation...
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS test-bundle1 --assume=yes"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Validating 3rd-party bundle binaries...
		No packs need to be downloaded
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Warning: File /bar/file_2 has dangerous permissions
		Warning: File /bar/file_3 has dangerous permissions
		Warning: The 3rd-party bundle you are about to install contains files with dangerous permission
		Do you want to continue? (y/N): y
		The "--assume=yes" option was used
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Exporting 3rd-party bundle binaries...
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
