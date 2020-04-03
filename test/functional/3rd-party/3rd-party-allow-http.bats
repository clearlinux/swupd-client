#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo "$TEST_NAME" 10 staging my_repo

	# use a web server for serving the content, this is necessary
	# since the code behaves differently if the content is local (e.g. file://)
	start_web_server -r -D "$TPWEBDIR"

}

test_teardown() {

	sudo rm -rf "$TARGETDIR"/etc/swupd

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "TPR005: Trying to add a http repo" {

	port=$(get_web_server_port "$TEST_NAME")

	run sudo sh -c "$SWUPD 3rd-party add my_repo http://localhost:$port $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
		Failed to add repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR006: Forcing a http repo add" {

	port=$(get_web_server_port "$TEST_NAME")

	run sudo sh -c "$SWUPD 3rd-party add my_repo http://localhost:$port --allow-insecure-http $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Adding 3rd-party repository my_repo...
	EOM
	)
	assert_in_output "$expected_output"
	assert_in_output "Repository added successfully"

	run sudo sh -c "cat $PATH_PREFIX/$THIRD_PARTY_DIR/repo.ini"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		[my_repo]
		url=http://localhost:$port
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/my_repo/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/my_repo/usr/lib/os-release
}
#WEIGHT=4
