#!/bin/bash

#TODO: Add test for crossing a format barrier

# shellcheck disable=SC1091
# SC1091: Not following, SC couldn't follow the dynamic path
# We already process that file, so it's fine to ignore
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
# $BASE_DIR:    A base dir different from current directory

export SWUPD_OPTS # Swupd options to use in tests
export SWUPD_OPTS_SHORT # Swupd options to use in tests (for bundle-list)
export FORMAT # Format of the last available version
export VERSION # Array of last available versions. First item is the oldest
export ROOT_DIR # Root directory of the installation

# get_format: Echos the format of the version in $1
get_format() {
	v="$1"
	curl -f "${URL}/${v}/format" 2>/dev/null
	return 0
}

# get_current_format(): Get last available format released
get_current_format() {
	last_version=$(curl -f "${URL}/version/latest_version" 2>/dev/null)
	get_format "$last_version"
	return 0
}

install_bundles() {
	local CMD=os-install
	# TODO: Replace verify --fix with repair as soon as the feature is implemented
	[ "$1" = "-r" ] && { CMD="verify --fix"; shift ; }

	if [ -z "$BUNDLE_LIST" ]; then
		run sudo sh -c "$SWUPD bundle-list --all $SWUPD_OPTS_SHORT --quiet"
		# shellcheck disable=SC2154
		# SC2154: output is referenced but not assigned.
		# the output variable is being assigned and exported by bats
		BUNDLE_LIST=$(echo "$output" | tr '\n' ' ')
		num_pkgs=$(echo "$output" | wc -l)

		cur_version=$(grep VERSION_ID "${ROOT_DIR}/usr/lib/os-release" | cut -d = -f 2)
		num_pkgs_mom=$(sh -c "curl ${URL}/$cur_version/Manifest.MoM 2>/dev/null | grep ^M\\\\. | wc -l")

		if [ "$num_pkgs" -ne "$num_pkgs_mom" ]; then
			print "Number of packages on bundle-list --all is $num_pkgs. Expected is $num_pkgs_mom:"
			print "$BUNDLE_LIST"
			return 1
		fi
	fi

	print "Install bundles: $BUNDLE_LIST"
	# shellcheck disable=SC2086
	# Disabling because I want to split spaces here
	CUR_VER=$("$SWUPD" info $SWUPD_OPTS | grep "Installed version"|cut -d ':' -f 2)
	BUNDLE_LIST="${BUNDLE_LIST// /,}"
	run sudo sh -c "$SWUPD $CMD $SWUPD_OPTS -B $BUNDLE_LIST -V $CUR_VER"
	print "Instalation completed"

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

	run sudo sh -c "$SWUPD diagnose --picky $SWUPD_OPTS 2>/dev/null"
	assert_status_is 0

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

	if [ -z "$BASE_DIR" ]; then
		BASE_DIR=$PWD
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
	FORMAT=$(get_current_format)
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

	ROOT_DIR="${BASE_DIR}/${TEST_NAME}"
	sudo rm -rf "$ROOT_DIR"
	sudo mkdir -p "$ROOT_DIR"

	SWUPD_OPTS_SHORT="-u ${URL} -p ${ROOT_DIR} -S ${ROOT_DIR}/var/lib/swupd/ --no-progress"
	SWUPD_OPTS="$SWUPD_OPTS_SHORT --no-scripts -t"
}

test_teardown() {
	if [ "$DEBUG_TEST" = true ]; then
		local msg="warning: debug_test is set to true, the test environment will be preserved"
		print "$msg\\n" 2>/dev/null || echo "$msg"
		return
	fi

	ROOT_DIR="${BASE_DIR}/${TEST_NAME}"
	sudo rm -rf "$ROOT_DIR"
}

