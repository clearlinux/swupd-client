#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

@test "ANL001: Shellcheck on changed files" {
	for i in $(git diff --name-only origin/master HEAD | grep -E "\.bats|\.bash"); do
		echo checking "$i" >&3

		"$BATS_TEST_DIRNAME"/../../scripts/shellcheck.bash "$i"
	done
}
