#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"
	# version 20
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --update /core
	# version 30
	create_version "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" os-core --add-dir /some_dir

}

@test "INS005: An OS install defaults to the latest OS version" {

	# if no version is specified when installing, it defaults to the latest

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 30 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 3 files
		  3 files were missing
		    3 of 3 missing files were installed
		    0 of 3 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core
	assert_dir_exists "$TARGETDIR"/some_dir

}

@test "INS006: User can specify \"latest\" OS version when installing" {

	# users can specify the "latest" version

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -V latest"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 30 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 3 files
		  3 files were missing
		    3 of 3 missing files were installed
		    0 of 3 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core
	assert_dir_exists "$TARGETDIR"/some_dir

}

@test "INS007: User can specify the OS version to install" {

	# users can specify the version to install

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -V 20"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 20
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installing base OS and selected bundles
		Inspected 2 files
		  2 files were missing
		    2 of 2 missing files were installed
		    0 of 2 missing files were not installed
		Calling post-update helper scripts
		Installation successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core

}
