#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a directory with some content that will
	# be used to create our test bundle
	sudo mkdir -p "$TEST_NAME"/tmp/dir_mode
	sudo mkdir -p "$TEST_NAME"/tmp/dir_owner
	sudo mkdir -p "$TEST_NAME"/tmp/dir_sticky
	write_to_protected_file "$TEST_NAME"/tmp/file_mode "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_owner "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_setuid "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_setgid "$(generate_random_content)"

	sudo chown -R 5001:5002 "$TEST_NAME"/tmp/*_owner
	sudo chmod -R 777 "$TEST_NAME"/tmp/*_mode

	print "Content directory in version 10:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the bundle using the content from tmp/
	create_bundle_from_content -t -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"
	# create a new version
	create_version "$TEST_NAME" 20 10

	# update file attributes
	sudo chown -R 5003:5004 "$TEST_NAME"/tmp/*_owner
	sudo chmod -R 707 "$TEST_NAME"/tmp/*_mode
	sudo chmod -R 4755 "$TEST_NAME"/tmp/file_setuid
	sudo chmod -R 2755 "$TEST_NAME"/tmp/file_setgid
	sudo chmod -R 1755 "$TEST_NAME"/tmp/dir_sticky

	# make different updates in the content
	print "\nContent directory in version 20:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the update for the bundle using the updated content
	update_bundle_from_content -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

}

@test "UPD070: Updating a system with multiple file attribute changes" {

	# updates should change all attributes of files, including mode and owner

	# Check user ID
	run stat --printf  "%u" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5001" ]
	run stat --printf  "%u" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5001" ]

	# Check group ID
	run stat --printf  "%g" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5002" ]
	run stat --printf  "%g" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5002" ]

	# Check mode
	run stat --printf  "%a" "$TARGET_DIR"/dir_mode
	[ "$output" -eq "777" ]
	run stat --printf  "%a" "$TARGET_DIR"/dir_mode
	[ "$output" -eq "777" ]

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 7
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		3 files were not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	# Check user
	run stat --printf  "%u" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5003" ]
	run stat --printf  "%u" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5003" ]

	# Check group
	run stat --printf  "%g" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5004" ]
	run stat --printf  "%g" "$TARGET_DIR"/dir_owner
	[ "$output" -eq "5004" ]

	# Check mode
	run stat --printf  "%a" "$TARGET_DIR"/dir_mode
	[ "$output" -eq "707" ]
	run stat --printf  "%a" "$TARGET_DIR"/dir_mode
	[ "$output" -eq "707" ]

	# Check extra attribute bits
	run stat --printf  "%a" "$TARGET_DIR"/file_setuid
	[ "$output" -eq "4755" ]
	run stat --printf  "%a" "$TARGET_DIR"/file_setgid
	[ "$output" -eq "2755" ]
	run stat --printf  "%a" "$TARGET_DIR"/dir_sticky
	[ "$output" -eq "1755" ]

	show_target

}
#WEIGHT=5
