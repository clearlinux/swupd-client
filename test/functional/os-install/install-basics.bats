#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

}

@test "INS001: Basic OS install" {

	# os-install will install a basic OS that has only os-core if nothing
	# else is specified

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
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

@test "INS002: An OS install can use --path to specify installation path" {

	# os-install will install a basic OS that has only os-core if nothing
	# else is specified

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH --path $TARGETDIR"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
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

@test "INS003: An OS install cannot use PATH and --path at the same time when installing" {

	# the swupd os-install command requires the path for the installation
	# to be provided by either using the --path option or simply specifying
	# it with no option, but not both

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH --path $TARGETDIR $TARGETDIR"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: cannot specify a PATH and use the --path option at the same time
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_not_exists "$TARGETDIR"/core

}
#WEIGHT=3
