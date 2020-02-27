#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1,/common "$TEST_NAME"
	file_hash=$(get_hash_from_manifest "$WEBDIR"/10/Manifest.test-bundle1 /common)
	file_path="$WEBDIR"/10/files/"$file_hash"
	create_bundle -L -n test-bundle2 -f /bar/test-file2,/common:"$file_path" "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /baz/test-file3,/common:"$file_path" "$TEST_NAME"
	create_bundle -n test-bundle4 -f /bat/test-file4,/common:"$file_path" "$TEST_NAME"
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
