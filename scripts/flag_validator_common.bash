#!/bin/bash

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWUPD_DIR="$SCRIPTS_DIR"/..
SWUPD="$SWUPD_DIR"/swupd

swupd_commands=(info autoupdate check-update update bundle-add bundle-remove
	bundle-list search-file diagnose repair os-install mirror clean hashdump)
conflict=0

check_command_exist() {

	# shellcheck disable=SC2116
	val=$(echo "sudo $SWUPD $1 --help > /dev/null 2>&1 && true")
	eval "$val"

}

count_global_flags() {

	sudo "$SWUPD" info --help | grep -oe "-.," | cut -c2 | wc -l

}

get_global_flags() {

	sudo "$SWUPD" info --help | grep -oe "-.," | cut -c2 | tr '\n' ' '

}

count_command_flags() {

	# shellcheck disable=SC2116
	val=$(echo "sudo $SWUPD $1 --help")
	eval "$val" | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | wc -l

}

get_command_flags() {

	# shellcheck disable=SC2116
	val=$(echo "sudo $SWUPD $1 --help")
	eval "$val" | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | tr '\n' ' '

}

find_global_flag_conflict() {

	local flags_count
	local unique_flags_count

	# global flags have to be compared only against other global flags
	flags_count=$(count_global_flags)
	unique_flags_count=$(get_global_flags | tr ' ' '\n' | sort -u | wc -l)
	if [ "$flags_count" != "$unique_flags_count" ]; then
		echo "Conflict found: There is a duplicated global flag"
		conflict=1
	fi

}

find_command_flag_conflict() {

	local command=$1
	local flags_count
	local unique_flags_count
	declare -a local_flags
	declare -a global_flags

	# first let's make sure there are no duplicated local flags
	flags_count=$(count_command_flags "$command")
	unique_flags_count=$(get_command_flags "$command" | tr ' ' '\n' | sort -u | wc -l)
	if [ "$flags_count" != "$unique_flags_count" ]; then
		echo "Conflict found: There is a duplicated flag in the $command command"
		conflict=1
	fi

	# local flags have to be compared against global flags too
	IFS=" " read -r -a global_flags <<< "$(get_global_flags)"
	IFS=" " read -r -a local_flags <<< "$(get_command_flags "$command")"
	for local_flag in "${local_flags[@]}"; do
		for global_flag in "${global_flags[@]}"; do
			if [ "$local_flag" == "$global_flag" ]; then
				echo "Conflict found: The flag '-$local_flag' from 'swupd $command' already exists as a global flag"
				conflict=1
			fi
		done
	done

}

validate_existing_flags() {

	find_global_flag_conflict

	for cmd in "${swupd_commands[@]}"; do
		# Commands which are not compiled dont have to be validated and
		# can be skipped eg: 3rd-party
		if ! check_command_exist "$cmd"; then
			continue
		fi
		find_command_flag_conflict "$cmd"
	done

	if [ "$conflict" -eq 1 ]; then
		exit 1
	fi
	echo "No conflicts found with the flags"

}

validate_flag() {

	local command=$1
	local cmd_flag=$2
	declare -a flags

	find_global_flag_conflict
	IFS=" " read -r -a flags <<< "$(get_global_flags)"
	for f in "${flags[@]}"; do
		if [ "$cmd_flag" == "$f" ]; then
			echo "Conflict found: The flag '-$f' from 'swupd $command' already exists as a global flag"
			exit 1
		fi
	done

	if [ "$command" != "global" ]; then
		find_command_flag_conflict "$command"
		IFS=" " read -r -a flags <<< "$(get_command_flags "$command")"
		for f in "${flags[@]}"; do
			if [ "$cmd_flag" == "$f" ]; then
				echo "Conflict found: The flag '-$f' already exists in 'swupd $command'"
				exit 1
			fi
		done
	fi

	echo "No conflicts found with the 'swupd $command -$cmd_flag' flag"

}
