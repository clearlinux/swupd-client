#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	# change the content of testfile so the hash doesn't match
	write_to_protected_file -a "$TARGETDIR"/usr/lib/kernel/testfile "some new content"
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
