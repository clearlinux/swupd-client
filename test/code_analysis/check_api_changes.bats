#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

@test "ANL005: swupd --quiet output has not changed" {

	run git diff --name-only --exit-code origin/master HEAD test/functional/api/

	# shellcheck disable=SC2154
	# SC2154: var is referenced but not assigned
	if [ "$status" -ne 0 ]; then
		echo "The following --quiet tests changed:"
		echo "$output"
		return 1
	fi

}
