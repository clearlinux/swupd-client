#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /usr/lib/kernel/testfile "$TEST_NAME"
	update_manifest -p "$WEBDIR"/10/Manifest.os-core file-status /usr/lib/kernel/testfile .db.
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /usr/lib/kernel/testfile "$zero_hash"
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
