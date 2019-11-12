#!/bin/bash

# shellcheck disable=SC1090
# SC1090: Can't follow non-constant source. Use a directive to specify location.
# We already process that file, so it's fine to ignore
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_PATH/../functional/testlib.bash"

set -e

#Check if there's no dupicated test
list_tests --all| cut -f 1 -d ':'|sort -cu

last_lbl=""
last_num=0
for t in $(list_tests --all| cut -f 1 -d ':'|sort); do
	lbl=$(echo "$t" | cut -b 1-3)
	num=$(echo "$t" | cut -b 4-6)
	if [ "$last_lbl" != "$lbl" ]; then
		last_lbl="$lbl"
		last_num=0
	fi

	last_num=$((last_num + 1))
	if [ "$last_num" -ne "$num" ]; then
		echo "Test $t has an incorrect number"
		exit 1
	fi
done

echo "All tests have valid IDs."

exit 0
