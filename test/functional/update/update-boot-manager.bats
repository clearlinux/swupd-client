#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	CONTENT="$TEST_NAME"/content_dir

	# content for bundle test-bundle1
	sudo mkdir -p "$CONTENT"/usr/bin
	write_to_protected_file "$CONTENT"/usr/bin/clr-boot-manager '#!/usr/bin/env bash\necho "hello"\n'
	sudo chmod +x "$CONTENT"/usr/bin/clr-boot-manager
	create_bundle_from_content -t -n test-bundle1 -c "$CONTENT" "$TEST_NAME"
	print "Content directory in version 10:\n\n $(tree "$CONTENT")"

	# content for update
	create_version -r "$TEST_NAME" 20 10
	write_to_protected_file -a "$CONTENT"/usr/bin/clr-boot-manager 'echo "world"\n'
	update_bundle_from_content -n test-bundle1 -c "$CONTENT" "$TEST_NAME"
	print "Content directory in version 20:\n\n $(tree "$CONTENT")"

}

@test "UPD072: Updating clr-boot-manager" {

	# an update that includes an update for clr-boot-manager should
	# trigger the clear-boot-manager script

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	 expected_output=$(cat <<-EOM
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
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		External command: hello
		External command: world
		1 file was not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
	show_target

}
