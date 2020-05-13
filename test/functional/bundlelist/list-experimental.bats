#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -e -t -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle    -e    -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle          -n test-bundle3 -f /file_3 "$TEST_NAME"

}

@test "LST014: List all available bundles including experimental" {

	# experimental bundles should be listed with something that distinguish them
	# from a normal bundle

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		All available bundles:
		 - os-core
		 - test-bundle1 (experimental)
		 - test-bundle2 (experimental)
		 - test-bundle3

		Total: 4
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "LST015: List all installed bundles including experimental" {

	# bundle-list with no options should list only installed bundles, there should
	# be a way to distinguish those that are experimental

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installed bundles:
		 - os-core
		 - test-bundle1 (experimental)

		Total: 2
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "LST016: List all installed bundles including experimental when the MoM fails to be retrieved" {

	# bundle-list with no options should list only installed bundles, there should
	# be a way to distinguish those that are experimental

	sudo mv "$STATEDIR"/manifest/10/Manifest.MoM "$STATEDIR"/manifest/10/Manifest.MoM.temp
	sudo mv "$WEBDIR"/10/Manifest.MoM.tar "$WEBDIR"/10/Manifest.MoM.tar.temp

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Error: Failed to retrieve 10 MoM manifest
		Warning: Could not determine which installed bundles are experimental
		Installed bundles:
		 - os-core
		 - test-bundle1
		Total: 2
	EOM
	)
	assert_in_output "$expected_output"

	sudo mv "$STATEDIR"/manifest/10/Manifest.MoM.temp "$STATEDIR"/manifest/10/Manifest.MoM
	sudo mv "$WEBDIR"/10/Manifest.MoM.tar.temp "$WEBDIR"/10/Manifest.MoM.tar

}

@test "LST025: List all installed bundles including experimental showing their installation status" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installed bundles:
		 - os-core (installed)
		 - test-bundle1 (explicitly installed, experimental)

		Total: 2
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=4
