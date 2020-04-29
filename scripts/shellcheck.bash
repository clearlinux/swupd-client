#!/bin/bash

# Wrapper to run shellcheck with configurations used by swupd project and
# doing necessary changes to validate bats files

# Ignored checks:
#  - SC1008: This shebang was unrecognized.
#    We need to skip this check when processing bats tests because the shebang
#    is not a shell.

if [ "$#" -ne 1 ]; then
	cat <<-EOM
		Invalid number of arguments

		Usage:
		   ./scripts/shellcheck.bash <shellscript_file>
	EOM
	exit 1
fi

file="$1"

if [[ "${file/*./}" == "bats" ]]; then
	sed 's/^@.*/func() {/' "$file" |
	sed 's/^load.*/source test\/functional\/testlib.bash/' |
	shellcheck -s bash -x -e SC1008 /dev/stdin
else
	shellcheck -x "$file"
fi
