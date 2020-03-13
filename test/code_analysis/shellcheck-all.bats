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

		run sh -c "sed 's/^@.*/func() {/' $i |
		sed 's/^load.*/source test\/functional\/testlib.bash/' |
		shellcheck -s bash -x -e SC1008 /dev/stdin"
		echo "Result: $status" >&3
		if [ ! "$status" -eq "0" ]; then
			echo "$output" >&3
			error=1
		fi
		echo >&3

	done

	[ "$error" -eq "0" ]
}
