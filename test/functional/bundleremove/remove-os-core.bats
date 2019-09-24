#!/usr/bin/env bats

load "../testlib"

@test "REM009: Try removing bundle 'os-core'" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS os-core"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/os-core
	assert_file_exists "$TARGETDIR"/core
	expected_output=$(cat <<-EOM
		Warning: Bundle "os-core" not allowed to be removed, skipping it...
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}