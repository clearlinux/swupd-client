#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	# create a 3rd-party repo with one installed bundle that has
	# multiple binaries
	create_test_environment -r "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle -L -n test-bundle1 -f /usr/bin/binary_1,/bin/binary_2,/usr/local/bin/binary_3 -u repo1 "$TEST_NAME"

	# create an update that updates only one of the bundle's binaries
	create_version -r "$TEST_NAME" 20 10 staging repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /bin/binary_2 repo1

	# let's add a line to the scripts for all binaries so we can tell if
	# they were re-generated after the update
	write_to_protected_file -a "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1 "TEST_STRING"
	write_to_protected_file -a "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2 "TEST_STRING"
	write_to_protected_file -a "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3 "TEST_STRING"

}

@test "TPR082: Update a 3rd-party bundle that has exported binaries when the template has not changed" {

	# pre-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3

	# If the template file didn't change then only the binary that was
	# udpated should be re-generated

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - os-core
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	# post-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_not_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	assert_in_output "TEST_STRING"

	# make sure the template file exists in the system and is correct
	assert_file_exists "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME"
	template_file=$(sudo cat "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME")
	template=$(echo -e "$TP_SCRIPT_TEMPLATE_CONTENT")
	assert_equal "$template_file" "$template"

}

@test "TPR083: Update a 3rd-party bundle that has exported binaries when there is no template" {

	# pre-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3

	# delete the template from the system
	sudo rm "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME"

	# If the template file is not found all binaries should be re-generated
	# regardless of if they were updated or not, also the template file
	# should be created

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - os-core
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
		The scripts that export binaries from 3rd-party repositories need to be regenerated
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Regenerating scripts...
		Scripts regenerated successfully
	EOM
	)
	assert_is_output "$expected_output"

	# post-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_not_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_not_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	assert_not_in_output "TEST_STRING"

	# make sure the template file exists in the system and is correct
	assert_file_exists "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME"
	template_file=$(sudo cat "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME")
	template=$(echo -e "$TP_SCRIPT_TEMPLATE_CONTENT")
	assert_equal "$template_file" "$template"

}

@test "TPR084: Update a 3rd-party bundle that has exported binaries when the template has changed" {

	# pre-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3

	# make a change to the template
	write_to_protected_file -a "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME" "random update"

	# If the template file is different than that in the swupd binary,
	# all binaries should be re-generated regardless of if they were updated or not,
	# also the template file should be updated

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - os-core
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
		The scripts that export binaries from 3rd-party repositories need to be regenerated
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Regenerating scripts...
		Scripts regenerated successfully
	EOM
	)
	assert_is_output "$expected_output"

	# post-test checks
	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_1
	assert_not_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_2
	assert_not_in_output "TEST_STRING"

	assert_file_exists "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/binary_3
	assert_not_in_output "TEST_STRING"

	run sudo cat "$TARGET_DIR"/"$TP_BIN_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME"
	assert_not_in_output "random update"

	# make sure the template file exists in the system and is correct
	assert_file_exists "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME"
	template_file=$(sudo cat "$TARGET_DIR"/"$TP_ROOT_DIR"/"$TP_SCRIPT_TEMPLATE_FILE_NAME")
	template=$(echo -e "$TP_SCRIPT_TEMPLATE_CONTENT")
	assert_equal "$template_file" "$template"

}
#WEIGHT=24
