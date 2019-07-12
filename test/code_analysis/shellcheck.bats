#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

@test "ANL001: Shellcheck on changed files" {
	for i in $(git diff --name-only origin/master HEAD | \grep -E "\.bats|\.bash"); do
		echo checking "$i"

		sed 's/^@.*/func() {/' "$i" |
		sed 's/^load.*/source test\/functional\/testlib.bash/' |
		shellcheck -s bash -x -e SC1008 /dev/stdin
	done
}
