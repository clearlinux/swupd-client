#!/bin/bash

#TODO: Add test for crossing a format barrier

SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../functional/testlib.bash"

# Test swupd with real official content published on clearlinux.org
# This test assumes all functional tests to be passing in order to work,
# because we aren't going to test all swupd functionalities. The idea is
# to test major swupd commands and standard use cases

# Optional parameters
# $URL: Clearlinux content and version urls
# $MAX_VERSIONS: The maximum number of versions we are going to be looking to
#                test updates. Should be # at least 2 (current + update).
#                Default is 10.
# $BUNDLE_LIST: List of bundles to be installed on the system besides os-core
#               and os-core-update. If empty, install all bundles.

export SWUPD_OPTS # Swupd options to use in tests
export SWUPD_OPTS_SHORT # Swupd options to use in tests (for bundle-list)
export FORMAT # Format of the last available version
export VERSION # Array of last available versions. First item is the oldest
export ROOT_DIR # Root directory of the installation

# get_format: Echos the format of the version in $1
# TODO: This is not the best way to handle that. we should have in swupd
# codebase the information about the supported format of this swupd.
get_format() {
	v="$1"
	curl -f "${URL}/${v}/format" 2>/dev/null
	return 0
}

# get_format_from_versions: Echos the first format available in a version from
# a list of versions
get_format_from_versions() {
	formats=("$@")
	i="${#formats[@]}"
	i=$((i-1))
	while [ "$i" -ge 0 ]; do
		format=$(get_format "${formats[$i]}")
		if [ -z "$format" ]; then # if version isn't published, discard
			continue
		else
			echo "$format"
			break
		fi
		i=$((i-1))
	done
}

install_bundles() {
	if [ -z "$BUNDLE_LIST" ]; then
		run sudo sh -c "$SWUPD bundle-list --all $SWUPD_OPTS_SHORT"
		BUNDLE_LIST=$(echo "$output" | tr '\n' ' ')
	fi

	print "Install bundles: $BUNDLE_LIST"

	run sudo sh -c "$SWUPD bundle-add --skip-diskspace-check $SWUPD_OPTS $BUNDLE_LIST"
	assert_status_is 0
	verify_system
}

# check_version: Check if version in /usr/lib/os-release is equal $1.
# Fails otherwise.
check_version() {
	version="$1"
	cur_version=$(grep VERSION_ID "${ROOT_DIR}/usr/lib/os-release" | cut -d = -f 2)
	if [ "$version" -ne "$cur_version" ]; then
		print "Version $version is different from expected $cur_version"
		return 1
	fi
}

verify_system() {

	run sudo sh -c "$SWUPD verify --picky $SWUPD_OPTS 2>/dev/null"
	if [ -n "$output" ]; then
		print "Verify found extra files in the system:"
		print "$output"
		return 1
	fi

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS"
	assert_not_in_output "did not match"

	return
}

test_setup() {
	# Fill defaults in optional parameters
	if [ -z "$URL" ]; then
		URL=https://cdn.download.clearlinux.org/update
	fi
	if [ -z "$MAX_VERSIONS" ]; then
		MAX_VERSIONS=10
	fi

	# Using github clr-bundles project to get official clear releases
	# version list is sorted from newer to older
	# TODO: Discover available versions to work with custom mixers
	read -r -d '\n' -a version_list < <(git ls-remote --tags https://github.com/clearlinux/clr-bundles.git | grep -v {} | grep -v latest | sed "s/.*refs\\/tags\\///" | sort -gr | head -n "$MAX_VERSIONS" | sort -g) || echo
	if [ "${#version_list[@]}" -eq 0 ]; then
		print "Impossible to get versions list"
		return 1
	fi

	# Get the last format
	FORMAT=$(get_format_from_versions "${version_list[@]}")
	export FORMAT

	# Fill a list of last available versions (up to ${MAX_VERSIONS})
	i=0
	for v in "${version_list[@]}"; do
		format=$(get_format "$v")
		# if version isn't published or is from a different version, discard
		if [ -z "$format" ] || [ "$FORMAT" -ne "$format" ]; then
			continue
		fi

		VERSION[$i]=$v
		i=$((i+1))
	done

	# Not enough versions to run the test
	if [ -z "${VERSION[0]}" ] || [ -z "${VERSION[1]}" ]; then
		# TODO: This is going to break on first release in a new format
		print "We need at least 2 versions in format $FORMAT to continue with this test"
		return 1
	fi

	sudo rm -rf "$TEST_NAME"
	sudo mkdir -p "$TEST_NAME"
	ROOT_DIR="${PWD}/${TEST_NAME}"

	# TODO: use -S ${ROOT_DIR}/var/lib/swupd/ when #665 gets fixed
	SWUPD_OPTS_SHORT="-u ${URL} -p ${ROOT_DIR} -S ${ROOT_DIR}/swupd/"
	SWUPD_OPTS="$SWUPD_OPTS_SHORT --no-scripts -t"
}

test_teardown() {
	sudo rm -rf "$TEST_NAME"
}

