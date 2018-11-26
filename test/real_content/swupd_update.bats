#!/usr/bin/env bats

load "real_content_lib"

# Test update in delta pack range
@test "RC001: Incremental Updates" {
	# shellcheck disable=SC2153
	echo "Install minimal system with oldest version (${VERSION[0]})"

	run sudo sh -c "$SWUPD verify --install $SWUPD_OPTS -m ${VERSION[0]} -F $FORMAT"
	assert_status_is 0
	check_version "${VERSION[0]}"

	echo "Install one package"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS os-core-update -F $FORMAT"
	assert_status_is 0
	verify_system

	install_bundles

	i=1
	while [ ! -z "${VERSION[$i]}" ]; do
		echo "Update system to next version (${VERSION[$i]})"
		run sudo sh -c "$SWUPD update $SWUPD_OPTS -m ${VERSION[$i]}"
		assert_status_is 0
		check_version "${VERSION[$i]}"
		verify_system

		i=$((i+1))
	done
}
