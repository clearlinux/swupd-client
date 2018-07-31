#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /usr/bin/file1 "$TEST_NAME"
	# modify the hash from the file with an incorrect one and re-create the file's tar
	manifest="$TEST_NAME"/web-dir/10/Manifest.test-bundle
	real_hash=$(get_hash_from_manifest "$manifest" "/usr/bin/file1")
	bad_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"
	sudo mv "$TEST_NAME"/web-dir/10/files/"$real_hash" "$TEST_NAME"/web-dir/10/files/"$bad_hash"
	sudo rm "$TEST_NAME"/web-dir/10/files/"$real_hash".tar
	create_tar "$TEST_NAME"/web-dir/10/files/"$bad_hash"
	# also modify it in the bundle manifest and re-create the manifest's tar
	sudo sed -i "s/$real_hash/$bad_hash/" "$manifest"
	sudo rm "$manifest".tar
	create_tar "$manifest"
	# finally update the MoM with the new manifest hash
	update_hashes_in_mom "$TEST_NAME"/web-dir/10/Manifest.MoM

}

@test "bundle-add add bundle containing file with different hash from what is listed in manifest" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	# downloaded fullfile had a bad hash - immediately fatal with a 1 return code
	assert_status_is 1
	# the bad hash file should not exist on the system
	assert_file_not_exists "$TEST_NAME"/target-dir/usr/bin/file1
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Error: File content hash mismatch for $TEST_DIRNAME/state/staged/e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3 (bad server data?)
	EOM
	)
	assert_is_output "$expected_output"

}

