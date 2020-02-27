#!/bin/bash

export TEST_FILENAME=$(basename "${BASH_SOURCE[0]}")
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../testlib.bash"
sudo rm -rf "$TEST_NAME"

create_test_environment -r "$TEST_NAME" 10 1
create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
create_version "$TEST_NAME" 20 10 1
update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3
set_current_version "$TEST_NAME" 20
# adding an untracked files into an untracked directory (/bat)
sudo mkdir "$TARGETDIR"/bat
sudo touch "$TARGETDIR"/bat/untracked_file1
# adding an untracked file into tracked directory (/bar)
sudo touch "$TARGETDIR"/bar/untracked_file2
# adding an untracked file into /usr
sudo touch "$TARGETDIR"/usr/untracked_file3

sudo tar cjf tests-cache/"$TEST_NAME".tar -C "$TEST_NAME" .
sudo rm -rf "$TEST_NAME"
