#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

	# Populate statedir-cache
	statedir_cache_path="$ABS_TEST_DIR"/testfs/statedir-cache
	sudo cp -r "$ABS_STATE_DIR" "$statedir_cache_path"
	cache_url=https___localhost
	sudo mkdir -m 755 -p "$statedir_cache_path"/cache/"$cache_url"/manifest/10
	sudo cp "$WEB_DIR"/10/Manifest.MoM "$statedir_cache_path"/cache/"$cache_url"/manifest/10
	sudo cp "$WEB_DIR"/10/Manifest.MoM.sig "$statedir_cache_path"/cache/"$cache_url"/manifest/10
	sudo cp "$WEB_DIR"/10/Manifest.os-core "$statedir_cache_path"/cache/"$cache_url"/manifest/10
	sudo touch "$statedir_cache_path"/cache/"$cache_url"/pack-os-core-from-0-to-10.tar
	sudo rsync -r "$WEB_DIR"/10/files/* "$statedir_cache_path"/cache/"$cache_url"/staged --exclude="*.tar"

}

@test "INS021: Install with populated statedir-cache and no network" {

	# The statedir-cache should be used to successfully perform the install
	# without an internet connection.

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR --url=https://localhost --statedir-cache $statedir_cache_path -V 10"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: Overriding version and content URLs with https://localhost
		Installing OS version 10
		Downloading missing manifests...
		No packs need to be downloaded
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
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGET_DIR"/core

}

@test "INS022: Install with missing manifest in statedir-cache and no network" {

	# Swupd should attempt to download the missing manifest and fail.

	sudo rm "$statedir_cache_path"/cache/"$cache_url"/manifest/10/Manifest.os-core
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR --url=https://localhost --statedir-cache $statedir_cache_path -V 10"

	assert_status_is "$SWUPD_COULDNT_LOAD_MANIFEST"
	expected_output=$(cat <<-EOM
		Warning: Overriding version and content URLs with https://localhost
		Installing OS version 10
		Downloading missing manifests...
		Error: Failed to connect to update server: https://localhost/10/Manifest.os-core.tar
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \\('clrtrust generate' may help\\)
		Error: Failed to retrieve 10 os-core manifest
		Error: Unable to download manifest os-core version 10, exiting now
		Installation failed
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_not_exists "$TARGET_DIR"/core

}

@test "INS023: Install with missing MoM signature in statedir-cache with no network" {

	# Swupd should attempt to download the signature and fail.

	sudo rm "$statedir_cache_path"/cache/"$cache_url"/manifest/10/Manifest.MoM.sig
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR --url=https://localhost --statedir-cache $statedir_cache_path -V 10"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Warning: Overriding version and content URLs with https://localhost
		Installing OS version 10
		Error: Failed to connect to update server: https://localhost/10/Manifest.MoM.sig
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \\('clrtrust generate' may help\\)
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Error: Failed to retrieve 10 MoM manifest
		Error: Unable to download/verify 10 Manifest.MoM
		Installation failed
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_not_exists "$TARGET_DIR"/core

}

@test "INS024: Install with missing fullfiles in statedir-cache with no network" {

	# Swupd should attempt to download the fullfiles and fail.

	sudo rm -r "$statedir_cache_path"/cache/"$cache_url"/staged
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR --url=https://localhost --statedir-cache $statedir_cache_path -V 10"

	assert_status_is "$SWUPD_COULDNT_DOWNLOAD_FILE"
	expected_output=$(cat <<-EOM
		Warning: Overriding version and content URLs with https://localhost
		Installing OS version 10
		Downloading missing manifests...
		No packs need to be downloaded
		Checking for corrupt files
		Validate downloaded files
		Error: Failed to connect to update server: https://localhost
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \\('clrtrust generate' may help\\)
		Error: Curl - Invalid parallel download handle
		Error: Unable to download necessary files for this OS release
		Installation failed
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_not_exists "$TARGET_DIR"/core

}

@test "INS025: Install with missing pack in statedir-cache with no network" {

	# Swupd should fail to download the pack, but continue successfully.

	sudo rm "$statedir_cache_path"/cache/"$cache_url"/pack-os-core-from-0-to-10.tar
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH $TARGET_DIR --url=https://localhost --statedir-cache $statedir_cache_path -V 10"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: Overriding version and content URLs with https://localhost
		Installing OS version 10
		Downloading missing manifests...
		Error: Failed to connect to update server: https://localhost/10/pack-os-core-from-0.tar
		Possible solutions for this problem are:
		.Check if your network connection is working
		.Fix the system clock
		.Run 'swupd info' to check if the urls are correct
		.Check if the server SSL certificate is trusted by your system \\('clrtrust generate' may help\\)
		Downloading packs for:
		 - os-core
		Error: zero pack downloads failed
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
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGET_DIR"/core

}
#WEIGHT=6
