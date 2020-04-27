#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com
@test "ANL004: Functions to avoid" {

	# There are some C functions that are tricky to use and it's prefered to
	# avoid using them in swupd. If any function in this list is really needed
	# we need to review the usage and add an exception here.
	local functions="strcpy wcscpy strcat wcscat sprintf vsprintf strtok ato strlen strcmp wcslen alloca vscanf vsscanf vfscanf scanf sscanf fscanf strncat strsep toa memmove asctime getwd gets basename dirname free system"

	local exceptions
	declare -A exceptions
	exceptions["basename"]="sys.*"
	exceptions["dirname"]="sys.*"
	exceptions["free"]="macros.h,verifytime.c,verifytime_main.c"

	local error=0
	for func in $functions; do
		local base_grep_cmd="grep \"\<$func(\" src --include \"*.c\" --include \"*.h\" -R"
		if [ -z "${exceptions[$func]}" ]; then
			run eval "$base_grep_cmd"
		else
			filters=$(echo "${exceptions[$func]}" | tr "," "\n")
			for i in $filters; do
				base_grep_cmd="$base_grep_cmd --exclude \"$i\""
			done
			run eval "$base_grep_cmd"
		fi

		# shellcheck disable=SC2154
		# SC2154: var is referenced but not assigned
		if [ "$status" -eq "0" ]; then
			echo "Found $func on files:"
			echo "$output"
			error=1
		fi
	done

	[ "$error" -eq "0" ]
}
