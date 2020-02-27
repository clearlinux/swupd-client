#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
create_test_environment "$TEST_NAME"
create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"
create_bundle -n test-bundle2 -f /test-file2 "$TEST_NAME"
create_version "$TEST_NAME" 100 10
update_bundle "$TEST_NAME" test-bundle1 --header-only
add_dependency_to_manifest "$WEBDIR"/100/Manifest.test-bundle1 test-bundle2
set_current_version "$TEST_NAME" 100
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
