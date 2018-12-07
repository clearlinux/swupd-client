#!/usr/bin/env bats

# Author: William Douglas
# Email: william.douglas@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n alias-bundle1 -f /foo/alias-file1 "$TEST_NAME"
	create_bundle -n alias-bundle2 -f /foo/alias-file2 "$TEST_NAME"
	create_bundle -n alias-bundle3 -f /foo/alias-file3 "$TEST_NAME"
}

test_setup() {

	return

}

test_teardown() {

	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle1
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.alias-bundle1
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.alias-bundle2
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.alias-bundle3
	sudo sh -c "rm -rf $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "rm -rf $TARGETDIR/etc/swupd/alias.d/"
	clean_state_dir "$TEST_NAME"

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "ADD033: Add bundle without alias" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1

}

@test "ADD034: Add bundle with system alias" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/usr/share/defaults/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}

@test "ADD035: Add bundle with user alias" {

	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}

@test "ADD036: Add bundles with alias (single file)" {

	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1\\talias-bundle2' > $TARGETDIR/etc/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle2, alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/foo/alias-file2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle2

}

@test "ADD037: Add bundles with aliases (single file, multi-line)" {

	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1\\nalias2\\talias-bundle2' > $TARGETDIR/etc/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1 alias2"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Alias alias2 will install bundle(s): alias-bundle2
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/foo/alias-file2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle2

}

@test "ADD038: Add bundles with aliases (multiple files)" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/a1"
	sudo sh -c "printf 'alias2\\talias-bundle2' > $TARGETDIR/usr/share/defaults/swupd/alias.d/a2"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1 alias2"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Alias alias2 will install bundle(s): alias-bundle2
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/foo/alias-file2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle2

}

@test "ADD039: Add bundle with alias (user mask)" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "ln -s /dev/null $TARGETDIR/etc/swupd/alias.d/a1"
	sudo sh -c "printf 'alias1\\talias-bundle2' > $TARGETDIR/usr/share/defaults/swupd/alias.d/a1"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/usr/share/defaults/swupd/alias.d/b1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}

@test "ADD040: Add bundle with alias (user override for different bundle)" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/a1"
	sudo sh -c "printf 'test-bundle1\\talias-bundle2' > $TARGETDIR/usr/share/defaults/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1 test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1

}

@test "ADD041: Add bundle with alias (user priority for different bundle)" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/b1"
	sudo sh -c "printf 'alias1\\talias-bundle2' > $TARGETDIR/usr/share/defaults/swupd/alias.d/a1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}

@test "ADD042: Add bundle with alias (name priority for different bundle)" {

	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/a1"
	sudo sh -c "printf 'alias1\\talias-bundle2' > $TARGETDIR/etc/swupd/alias.d/b1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}

@test "ADD043: Add bundle with alias (user+name priority for different bundle)" {

	sudo sh -c "mkdir -p $TARGETDIR/usr/share/defaults/swupd/alias.d/"
	sudo sh -c "mkdir -p $TARGETDIR/etc/swupd/alias.d/"
	sudo sh -c "printf 'alias1\\talias-bundle1' > $TARGETDIR/etc/swupd/alias.d/a1"
	sudo sh -c "printf 'alias1\\talias-bundle2' > $TARGETDIR/etc/swupd/alias.d/b1"
	sudo sh -c "printf 'alias1\\talias-bundle3' > $TARGETDIR/usr/share/defaults/swupd/alias.d/b1"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS alias1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Alias alias1 will install bundle(s): alias-bundle1
		Starting download of remaining update content. This may take a while...
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"
	assert_file_exists "$TARGETDIR"/foo/alias-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/alias-bundle1

}
