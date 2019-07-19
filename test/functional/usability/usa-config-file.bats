#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /file_1 "$TEST_NAME"

}

test_setup() {

	create_config_file

}

test_teardown() {

	destroy_config_file

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "USA010: Use option without section in the configuration file" {

	# Options without a section are considered to be global

	add_option_to_config_file json_output true

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "bundle-add", "status" : 0 }
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "bundle-remove", "status" : 0 }
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA011: Set a global option in the configuration file" {

	# Global options apply to all commands

	add_option_to_config_file json_output true GLOBAL

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "info", "status" : 0 }
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "check-update", "status" : 1 }
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA012: Set a global option locally for a command in the configuration file" {

	# Global options defined within a command's section only apply to that command

	add_option_to_config_file json_output true info

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "info", "status" : 0 }
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA013: Set a global option and override it locally for a command in the configuration file" {

	# If an option is defined globally and locally, local values should take precedence

	add_option_to_config_file json_output true global
	add_option_to_config_file json_output false check-update

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		{ "type" : "end", "section" : "info", "status" : 0 }
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA014: Set a local option  in a command in the configuration file" {

	# options set within the command section apply to the command only

	add_option_to_config_file picky true diagnose

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Generating list of extra files under $TEST_DIRNAME/testfs/target-dir/usr
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA015: Override a global option from the configuration file with a command flag" {

	# flags used with a command take precedence over options in the config file

	add_option_to_config_file url http://someurl.com global

	run sudo sh -c "$SWUPD info $SWUPD_OPTS -v http://anotherurl.com"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Distribution:      Clear Linux Software for Intel Architecture
		Installed version: 10
		Version URL:       http://anotherurl.com
		Content URL:       http://someurl.com
	EOM
	)
	assert_is_output "$expected_output"

}

@test "USA016: Override a local option from the configuration file with a command flag" {

	# flags used with a command take precedence over options in the config file

	add_option_to_config_file picky true diagnose
	add_option_to_config_file picky_tree /fake/path

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky-tree /usr/lib"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Generating list of extra files under $TEST_DIRNAME/testfs/target-dir/usr/lib
	EOM
	)
	assert_in_output "$expected_output"

}

@test "USA017: Use an invalid section in the configuration file" {

	# if invalid sections are found within the config file it should be ignored

	add_option_to_config_file json_output true invalid_section

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Distribution:      Clear Linux Software for Intel Architecture
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

}

@test "USA018: Use an invalid option within a valid section in the configuration file" {

	# if invalid option for a valid section is found within the config file
	# a warning should be generated and the option should be ignored

	add_option_to_config_file invalid_option true global

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: Unrecognized option 'invalid_option=true' from section [global] in the configuration file
		Distribution:      Clear Linux Software for Intel Architecture
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)
	assert_is_output "$expected_output"

}

@test "USA019: Use a bad option in the configuration file" {

	# if a bad/corrupt option for a valid section is found within the config file
	# a warning should be generated and the option should be ignored

	add_option_to_config_file picky true diagnose
	{ echo "picky-tree="; echo "=true"; echo "picky-tree=/some/path=/usr"; } >> "$SWUPD_CONFIG_FILE"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Warning: Invalid key/value pair in the configuration file (key='picky-tree', value='')
		Warning: Invalid key/value pair in the configuration file (key='', value='true')
		Warning: Invalid key/value pair in the configuration file ('picky-tree=/some/path=/usr')
		Diagnosing version 10
		Generating list of extra files under $TEST_DIRNAME/testfs/target-dir/usr
	EOM
	)
	assert_in_output "$expected_output"

}
