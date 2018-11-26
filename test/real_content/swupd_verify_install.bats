#!/usr/bin/env bats

load "real_content_lib"

# Test massive fullfile downloads
@test "RC004: swupd verify install with all bundles" {
	i=$((NUM_VERSIONS-1))
	sudo mkdir -p "${ROOT_DIR}/usr/lib"
	sudo mkdir -p "${ROOT_DIR}/usr/share/clear/bundles"

	if [ -z "$BUNDLE_LIST" ]; then
		run sudo sh -c "$SWUPD bundle-list --all $SWUPD_OPTS_SHORT -F $FORMAT"
		BUNDLE_LIST=$(echo "$output" | tr '\n' ' ')
	fi
	for b in $BUNDLE_LIST; do
		sudo touch "$ROOT_DIR/usr/share/clear/bundles/$b"
	done

	# shellcheck disable=SC2153
	print "Install complete system with newest version (${VERSION[$i]})"
	run sudo sh -c "$SWUPD verify --install $SWUPD_OPTS -m ${VERSION[$i]} -F $FORMAT"
	assert_status_is 0
	check_version "${VERSION[$i]}"
	verify_system
}
