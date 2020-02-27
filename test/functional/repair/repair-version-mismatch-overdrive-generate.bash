#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -d /usr/bin "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" os-core --header-only
	set_current_version "$TEST_NAME" 20
	# remove /usr/bin so it is missing in the target system
	sudo rm -rf "$TARGETDIR"/usr/bin
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
