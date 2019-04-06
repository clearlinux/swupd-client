#!/usr/bin/bash

FUNC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$FUNC_DIR"/testlib.bash

declare -A groups=( ["bundleadd"]="ADD" ["bundlelist"]="LST" ["bundleremove"]="REM" \
	["checkupdate"]="CHK" ["hashdump"]="HSD" ["mirror"]="MIR" ["search"]="SRH" \
	["update"]="UPD" ["usability"]="USA" ["verify"]="VER" )

invalid=false

for group in "${!groups[@]}"; do

	group_code="${groups[$group]}"
	num_tests=$(list_tests "$FUNC_DIR/$group" | wc -l)
	num_unique_tests=$(list_tests "$FUNC_DIR/$group" | cut -d ":" -f 1 | sort -u | wc -l)

	for iter in $(seq -f "%03g" 1 "$num_tests"); do

	        found=false
		for id in $(list_tests "$FUNC_DIR/$group" | cut -d ":" -f 1 | sort -u)
	        do
			if [ "$id" = "$group_code$iter" ]; then
	                        found=true
	                        break
	                fi
	        done

		if [ "$found" = false ]; then
			echo "A test with ID $group_code$iter was not found."
			invalid=true
		fi

	done

	if [ "$num_tests" -ne "$num_unique_tests" ]; then

	        dup=$(expr "$num_tests" - "$num_unique_tests")
	        echo "Found $dup tests with duplicated IDs in '$group'."
	        invalid=true

	fi

done

if [ "$invalid" = true ]; then

	echo -e "\nRun 'list_tests --all' to check the test IDs."
	exit 1

fi

echo "All tests have valid IDs."

exit 0
