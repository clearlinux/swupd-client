#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

@test "ANL002: Shellcheck on all files" {
	find . -regex ".*\.bats\|.*\.bash" -print0 | while IFS= read -d '' -r i; do
		echo checking "$i"

		sed 's/^@.*/func() {/' "$i" |
		sed 's/^load.*/source test\/functional\/testlib.bash/' |
		shellcheck -s bash -x -e SC1008 /dev/stdin
	done
}
