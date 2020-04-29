#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

@test "ANL002: Shellcheck on all files" {
	local error=0
	status=0
	output=""
	TESTS=$(find . -regex ".*\\.bats\\|.*\\.bash" | sort)
	for i in $TESTS; do
		echo checking "$i" >&3

		run "$BATS_TEST_DIRNAME"/../../scripts/shellcheck.bash "$i"
		echo "Result: $status" >&3
		if [ ! "$status" -eq "0" ]; then
			echo "$output" >&3
			error=1
		fi
		echo >&3

	done

	[ "$error" -eq "0" ]
}
