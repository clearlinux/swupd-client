#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1

	file_1=$(create_file "$TP_WEB_DIR"/10/files)
	# create a file with a dangerous flag (setuid)
	file_2=$(create_file -u "$TP_WEB_DIR"/10/files)

	# create and install a bundle that already has a file with
	# a dangerous flag (file_2)
	create_bundle -L -t -n test-bundle1 -f /foo/file_1:"$file_1",/file_2:"$file_2",/foo/file_3 -u repo1 "$TEST_NAME"

	# create an update that adds a new file with dangerous flags, and
	# that changes the flags of an existing file with dangerous ones
	create_version -p "$TEST_NAME" 20 10 staging repo1
	sudo chmod g+s "$file_1"
	file_4=$(create_file -u "$TP_WEB_DIR"/20/files)

	# file_1 was not dangerous, it now is
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1:"$file_1" repo1
	# file_2 was dangerous but was already installed (so the assumption is that it
	# had been previously accepted, no new warning should be generatde)
	update_bundle -p "$TEST_NAME" test-bundle1 --update /file_2               repo1
	# file_3 was updated but is not dangerous
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_3           repo1
	# file_4 is a new dangerous file being added with the update
	update_bundle    "$TEST_NAME" test-bundle1 --add    /bar/file_4:"$file_4" repo1

}

@test "TPR062: Updating a system from a 3rd-party repo when the update has files with dangerous flags - confirm installation" {

	# when updating 3rd-party content, if there are new files with
	# dangerous flags in the update the update must be paused and should
	# only proceed if the user accepts the risk

	run sudo sh -c "echo 'y' | $SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 2
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Warning: The update has a new file /bar/file_4 with dangerous permissions
		Warning: The update sets dangerous permissions to file /foo/file_1
		Warning: The 3rd-party update you are about to install contains files with dangerous permission
		Do you want to continue? (y/N):$SPACE
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR063: Try updating a system from a 3rd-party repo when the update has files with dangerous flags - cancell installation" {

	# if the user doesn't accept the risk, the update should be aborted

	run sudo sh -c "echo 'n' | $SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_FILE"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 2
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Warning: The update has a new file /bar/file_4 with dangerous permissions
		Warning: The update sets dangerous permissions to file /foo/file_1
		Warning: The 3rd-party update you are about to install contains files with dangerous permission
		Do you want to continue? (y/N):$SPACE
		Aborting update...
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR064: Try updating a system from a 3rd-party repo when the update has files with dangerous flags using --assume to run non interactively" {

	# the --non-interactive flag can be used to avoid getting prompots
	# from swupd

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --assume=no"

	assert_status_is "$SWUPD_INVALID_FILE"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 2
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Warning: The update has a new file /bar/file_4 with dangerous permissions
		Warning: The update sets dangerous permissions to file /foo/file_1
		Warning: The 3rd-party update you are about to install contains files with dangerous permission
		Do you want to continue? (y/N): N
		The "--assume=no" option was used
		Aborting update...
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --yes"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 2
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Warning: The update has a new file /bar/file_4 with dangerous permissions
		Warning: The update sets dangerous permissions to file /foo/file_1
		Warning: The 3rd-party update you are about to install contains files with dangerous permission
		Do you want to continue? (y/N): y
		The "--assume=yes" option was used
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=20
