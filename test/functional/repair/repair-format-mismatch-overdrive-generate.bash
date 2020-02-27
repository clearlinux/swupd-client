#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2
	update_bundle "$TEST_NAME" os-core --add-dir /usr/bin
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
