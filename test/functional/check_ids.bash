#!/bin/bash

FUNC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=/dev/null
source "$FUNC_DIR"/testlib.bash

declare -A groups=( ["bundleadd"]="ADD" ["bundlelist"]="LST" ["bundleremove"]="REM" \
	["checkupdate"]="CHK" ["hashdump"]="HSD" ["mirror"]="MIR" ["search"]="SRH" \
	["update"]="UPD" ["usability"]="USA" ["verify"]="VER" )

invalid=false

for group in "${!groups[@]}"; do

	group_code="${groups[$group]}"
	num_tests=$(list_tests "$FUNC_DIR/$group" | wc -l)

	for iter in $(seq -f "%03g" 1 "$num_tests"); do

		found=0

		for id in $(list_tests "$FUNC_DIR/$group" | cut -d ":" -f 1 | sort); do
			if [ "$group_code$iter" = "$id" ]; then
				found=$((found + 1))
			fi
		done

		if [ "$found" -eq 0 ]; then
			echo "A test with ID '$group_code$iter' was not found."
			invalid=true
		elif [ "$found" -gt 1  ]; then
			echo "Found $found tests with duplicated IDs in '$group'."
			invalid=true
		fi

	done

done

if [ "$invalid" = true ]; then

	echo -e "\nRun 'list_tests --all' to check the test IDs."
	exit 1

fi

echo "All tests have valid IDs."

exit 0
