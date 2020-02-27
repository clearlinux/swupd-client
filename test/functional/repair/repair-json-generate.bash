#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1,/bar/test-file2,/baz "$TEST_NAME"
	create_bundle -n test-bundle2 -f /test-file3,/test-file4 "$TEST_NAME"
	sudo rm -f "$TARGETDIR"/foo/test-file1
	sudo rm -f "$TARGETDIR"/baz
	sudo touch "$TARGETDIR"/usr/extraneous-file
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
