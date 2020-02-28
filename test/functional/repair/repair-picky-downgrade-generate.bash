#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	create_bundle -L -n test-bundle2 -f /usr/foo/file_2,/usr/foo/file_3,/bar/file_4,/bar/file_5 "$TEST_NAME"
	set_current_version "$TEST_NAME" 20
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"