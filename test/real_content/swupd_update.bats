#!/usr/bin/env bats

load "real_content_lib"

# Test update in delta pack range
@test "RC001: Incremental Updates" {
	# shellcheck disable=SC2153
	print "Install minimal system with oldest version (${VERSION[0]})"

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS -V ${VERSION[0]} -F $FORMAT"
	assert_status_is 0
	check_version "${VERSION[0]}"

	print "Install one package"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS os-core-update -F $FORMAT"
	assert_status_is 0
	verify_system

	install_bundles

	i=1
	while [ ! -z "${VERSION[$i]}" ]; do
		print "Update system to next version (${VERSION[$i]})"
		run sudo sh -c "$SWUPD update $SWUPD_OPTS -m ${VERSION[$i]}"
		assert_status_is 0
		check_version "${VERSION[$i]}"
		verify_system

		i=$((i+1))
	done
}
