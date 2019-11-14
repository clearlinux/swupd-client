#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

check_sort_makefile()
{
	start_str="$1"
	export LC_COLLATE=en_US.UTF-8
	START=$(grep "$start_str" Makefile.am --line-number -m 1|  cut -f1 -d:)
	START=$((START + 1))
	END=$(tail +"$START"  Makefile.am | grep -e '^$' --line-number -m 1 |  cut -f1 -d:)
	END=$((END - 1))
	tail -n +$START Makefile.am | head -n $END | sort -c
}

@test "ANL003: Compliant code style check" {
	run git diff --quiet --exit-code src
	# shellcheck disable=SC2154
	# output and status variable is being assigned and exported by bats
	if [ "$status" -ne 0 ]; then
		echo "Error: can only check code style when src/ is clean."
		echo "Stash or commit your changes and try again."
		return "$status"
	fi

	run clang-format-9 -i -style=file src/*.[ch] src/lib/*.[ch]
	if [ "$status" -ne 0 ]; then
		echo "clang-format-9 failed with status $status. Check if you have clang-format-9 installed"
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

	run grep -r '[[:blank:]]$' test/ src/ -l --include \*.bats --include \*.c --include \*.h
	if [ "$status" -eq 0 ]; then
		echo "Trailing whitespaces in files:"
		echo "$output"
		return 1
	fi

	check_sort_makefile swupd_SOURCES
	check_sort_makefile "TESTS ="
}
