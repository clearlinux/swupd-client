#!/usr/bin/env bats

load "real_content_lib"

# Test update out of the delta pack range
@test "RC003: Update from 2 distant versions" {
	# shellcheck disable=SC2153
	print "Install minimal system with oldest version (${VERSION[0]})"
	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -V ${VERSION[0]} -F $FORMAT"
	assert_status_is 0
	check_version "${VERSION[0]}"

	install_bundles

	version=${VERSION[${#VERSION[@]} -1]}

	print "Update system to last version ($version)"
	run sudo sh -c "$SWUPD update $SWUPD_OPTS -m ${version}"
	assert_status_is 0
	check_version "$version"
	verify_system
}
