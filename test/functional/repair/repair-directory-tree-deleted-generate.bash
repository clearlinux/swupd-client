#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /testdir1/testdir2/testfile "$TEST_NAME"
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-status /testdir1/testdir2/testfile .d..
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-status /testdir1/testdir2 .d..
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-status /testdir1 .d..
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-hash /testdir1/testdir2/testfile "$zero_hash"
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-hash /testdir1/testdir2 "$zero_hash"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /testdir1 "$zero_hash"
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
