#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

@test "ANL003: Compliant code style check" {
	run git diff --quiet --exit-code src
	# shellcheck disable=SC2154
	# output and status variable is being assigned and exported by bats
	if [ "$status" -ne 0 ]; then
		echo "Error: can only check code style when src/ is clean."
		echo "Stash or commit your changes and try again."
		return "$status"
	fi

	run clang-format -i -style=file src/*.[ch] src/lib/*.[ch]
	if [ "$status" -ne 0 ]; then
		return "$status"
	fi

	run git diff --quiet --exit-code src
	if [ "$status" -ne 0 ]; then
		echo "Code style issues found. Run 'git diff' to view issues."
		return "$status"
	fi

	run git grep if\ \(.*\)$ -- '*.c'
	if [ "$status" -eq 0 ]; then
		echo "Missing brackets in single line if:"
		# shellcheck disable=SC2154
		# output and status variable is being assigned and exported by bats
		echo "$output"
		return 1
	fi
}
