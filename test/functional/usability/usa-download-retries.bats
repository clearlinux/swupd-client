#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /file_1 "$TEST_NAME"
	# start a web server that will return status code 204 for all requests
	start_web_server -r -D "$WEBDIR" -f 204
	port=$(get_web_server_port "$TEST_NAME")
	# set the versionurl and contenturl to point to the web server
	set_upstream_server "$TEST_NAME" http://localhost:"$port"

}

@test "USA005: Retries for failed downloads can be configured" {

	# Users should be able to configure the retry of a failed download using
	# two options --max-retries and --retry-delay

	run sudo sh -c "$SWUPD bundle-add --max-retries 4 --retry-delay 0 $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Retry #1 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Retry #2 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Retry #3 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Retry #4 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Warning: Maximum number of retries reached
		Error: Failed to retrieve 10 MoM manifest
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_is_output "$expected_output"

}

@test "USA006: Retries for failed downloads can be disabled" {

	# Users should be able to turn off download retries entirely by setting the
	# value to 0

	run sudo sh -c "$SWUPD bundle-add --max-retries 0 $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Download retries is disabled
		Error: Failed to retrieve 10 MoM manifest
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_is_output "$expected_output"

}

@test "USA007: Retries for failed downloads defaults to three retries" {

	# All failed downloads by default are atttempted 3 times

	run sudo sh -c "$SWUPD bundle-add --retry-delay 1 $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Waiting 1 seconds before retrying the download
		Retry #1 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Waiting 2 seconds before retrying the download
		Retry #2 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Waiting 4 seconds before retrying the download
		Retry #3 downloading from http://localhost:$port/10/Manifest.MoM.tar
		Error: Curl - Download failed: response (204) -  'http://localhost:$port/10/Manifest.MoM.tar'
		Warning: Maximum number of retries reached
		Error: Failed to retrieve 10 MoM manifest
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_is_output "$expected_output"

}
