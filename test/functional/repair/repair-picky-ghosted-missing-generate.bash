#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"
	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/foo -d/usr/share/clear/bundles "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.test-bundle file-status /usr/foo .g..
	# since the files must have been installed by the -L option, remove /usr/foo
	sudo rm -f "$TARGETDIR"/usr/foo
sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
