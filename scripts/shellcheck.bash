#!/bin/bash

# Wrapper to run shellcheck with configurations used by swupd project and
# doing necessary changes to validate bats files

# Ignored checks:
#  - SC1008: This shebang was unrecognized.
#    We need to skip this check when processing bats tests because the shebang
#    is not a shell.
# - SC2119: Use foo "$@" if function's $1 should mean script's $1.
#   We need to skip this check because in many ocassions we use functions
#   that are not expecting arguments, but still shellcheck will detect this
#   as error because the show_help which is called from all functions uses $@

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
	shellcheck -s bash -x -e SC1008,SC2119,SC2317 /dev/stdin
else
	shellcheck -x "$file" -e SC2119
fi
