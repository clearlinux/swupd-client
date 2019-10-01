#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

	statedir_cache_path="${TEST_DIRNAME}/testfs/statedir-cache"

	# Populate statedir-cache
	sudo mkdir -m 700 -p "$statedir_cache_path"
	sudo mkdir -m 700 "$statedir_cache_path"/staged
	sudo mkdir -m 755 "$statedir_cache_path"/10
	sudo cp "$WEBDIR"/10/Manifest.MoM "$statedir_cache_path"/10
	sudo cp "$WEBDIR"/10/Manifest.MoM.sig "$statedir_cache_path"/10
	sudo cp "$WEBDIR"/10/Manifest.os-core "$statedir_cache_path"/10
	sudo touch "$statedir_cache_path"/pack-os-core-from-0-to-10.tar
	sudo rsync -r "$WEBDIR"/10/files/* "$statedir_cache_path"/staged --exclude="*.tar"

}

@test "INS017: Install with populated statedir-cache and valid network" {

	# The statedir-cache should be used for all update content, so the
	# network should be unused.

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR --statedir-cache $statedir_cache_path"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		No packs need to be downloaded
		Checking for corrupt files
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

@test "INS018: Install with no update content in the statedir-cache and valid network" {

	# The statedir-cache will have no update content, so the network must be used
	# as a fallback.

	sudo rm "$statedir_cache_path"/10/Manifest.os-core
	sudo rm "$statedir_cache_path"/pack-os-core-from-0-to-10.tar
	sudo rm -r "$statedir_cache_path"/staged
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR --statedir-cache $statedir_cache_path"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
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

@test "INS019: Install with missing fullfiles in statedir-cache and valid network" {

	# When fullfiles are missing from the statedir-cache, downloads over the network
	# must be used to sucessfully install. The goal of this test case is to verify that
	# missing fullfiles in the statedir-cache will fallback to network downloads, so packs
	# must not be used to populate the staged directory with fullfiles.

	sudo rm -r "$statedir_cache_path"/staged
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR --statedir-cache $statedir_cache_path"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		No packs need to be downloaded
		Checking for corrupt files
		Starting download of remaining update content. This may take a while...
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

@test "INS020: Install with corrupt manifest in statedir-cache and valid network" {

	# Swupd should fallback to network downloads when the statedir-cache contains
	# corrupt manifests.

	sudo sh -c "echo invalid > ${statedir_cache_path}/10/Manifest.os-core"
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGETDIR --statedir-cache $statedir_cache_path"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Warning: hash check failed for Manifest.os-core for version 10. Deleting it
		Warning: Removing corrupt Manifest.os-core artifacts and re-downloading...
		No packs need to be downloaded
		Checking for corrupt files
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
