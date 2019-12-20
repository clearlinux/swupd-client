#!/bin/bash

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWUPD_DIR="$SCRIPTS_DIR"/..
SWUPD="$SWUPD_DIR"/swupd

swupd_commands=(info autoupdate check-update update bundle-add bundle-remove bundle-list bundle-info search-file diagnose repair os-install mirror clean hashdump 3rd-party)
third_party_commands=(add remove list bundle-add bundle-remove bundle-list bundle-info update diagnose repair check-update clean)
conflict=0

count_global_flags() {

	sudo "$SWUPD" info --help | grep -oe "-.," | cut -c2 | wc -l

}

get_global_flags() {

	sudo "$SWUPD" info --help | grep -oe "-.," | cut -c2 | tr '\n' ' '

}

count_command_flags() {

	local command=$1
	local subcommand=$2

	if [ -n "$subcommand" ]; then
		sudo "$SWUPD" "$command" "$subcommand" --help | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | wc -l
	else
		sudo "$SWUPD" "$command" --help | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | wc -l
	fi

}

get_command_flags() {

	local command=$1
	local subcommand=$2

	if [ -n "$subcommand" ]; then
		sudo "$SWUPD" "$command" "$subcommand" --help | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | tr '\n' ' '
	else
		sudo "$SWUPD" "$command" --help | sed -n '/^Options:/,$p' | grep -oe "-.," | cut -c2 | tr '\n' ' '
	fi

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
	local subcommand=$2
	local flags_count
	local unique_flags_count
	local CMD
	declare -a local_flags
	declare -a global_flags

	if [ -n "$subcommand" ]; then
		CMD="$command $subcommand"
	else
		CMD="$command"
	fi

	# first let's make sure there are no duplicated local flags
	flags_count=$(count_command_flags "$command" "$subcommand")
	unique_flags_count=$(get_command_flags "$command" "$subcommand" | tr ' ' '\n' | sort -u | wc -l)
	if [ "$flags_count" != "$unique_flags_count" ]; then
		echo "Conflict found: There is a duplicated flag in the $CMD command"
		conflict=1
	fi

	# local flags have to be compared against global flags too
	IFS=" " read -r -a global_flags <<< "$(get_global_flags)"
	IFS=" " read -r -a local_flags <<< "$(get_command_flags "$command" "$subcommand")"
	for local_flag in "${local_flags[@]}"; do
		for global_flag in "${global_flags[@]}"; do
			if [ "$local_flag" == "$global_flag" ]; then
				echo "Conflict found: The flag '-$local_flag' from 'swupd $CMD' already exists as a global flag"
				conflict=1
			fi
		done
	done

}

validate_existing_flags() {

	local cmd

	find_global_flag_conflict

	for cmd in "${swupd_commands[@]}"; do
		find_command_flag_conflict "$cmd"
	done

	for cmd in "${third_party_commands[@]}"; do
		find_command_flag_conflict 3rd-party "$cmd"
	done

	if [ "$conflict" -eq 1 ]; then
		exit 1
	fi
	echo "No conflicts found with the flags"

}

validate_flag() {

	local command=$1
	local subcommand=$2
	local cmd_flag=$3
	local CMD
	declare -a flags

	if [ -n "$subcommand" ]; then
		CMD="$command $subcommand"
	else
		CMD="$command"
	fi

	find_global_flag_conflict
	IFS=" " read -r -a flags <<< "$(get_global_flags)"
	for f in "${flags[@]}"; do
		if [ "$cmd_flag" == "$f" ]; then
			echo "Conflict found: The flag '-$f' from 'swupd $CMD' already exists as a global flag"
			exit 1
		fi
	done

	if [ "$command" != "global" ]; then
		find_command_flag_conflict "$command" "$subcommand"
		IFS=" " read -r -a flags <<< "$(get_command_flags "$command" "$subcommand")"
		for f in "${flags[@]}"; do
			if [ "$cmd_flag" == "$f" ]; then
				echo "Conflict found: The flag '-$f' already exists in 'swupd $CMD'"
				exit 1
			fi
		done
	fi

	echo "No conflicts found with the 'swupd $CMD -$cmd_flag' flag"

}

usage() {

	cat <<-EOM

		Validates that the flags used in swupd are not duplicated

		Usage:
		    flag_validator.bash <command> <flag>

		Example:
		    flag_validator.bash bundle-add w

		Notes:
		    If run with no argument, it validates the existing flags
		    If run with the <command> <flag> arguments, it validate that <flag> is a valid option for <command>
		    If <flag> is a global option, use "global <flag>"

	EOM

}

# Entry point
[[ "${BASH_SOURCE[0]}" != "${0}" ]] && return  # the file is being sourced, return now ( do not "exit", it would exit the shell)

command=$1
if [ "$command" = 3rd-party ]; then
	subcommand=$2
	flag=$3
else
	subcommand=""
	flag=$2
fi

if [ "$command" == "--help" ] || [ "$command" == "-h" ]; then
	usage
	exit
fi

# If run with no argument, it validates the existing flags
if [ -z "$command" ]; then
	validate_existing_flags
	exit
fi

# If run with the <command> <flag> arguments, it validates
# that <flag> is a valid option for <command>
if [ -z "$flag" ]; then
	echo "Error: Mandatory argument missing"
	usage
	exit 1
fi

swupd_commands+=("global")
for val in "${swupd_commands[@]}"; do
	if [ "$val" == "$command" ]; then
		# remove the '-' if it has one
		flag=$(echo "$flag" | tr --delete '-')
		validate_flag "$command" "$subcommand" "$flag"
		exit "$conflict"
	fi
done

echo "Error: The provided command '$command' is invalid"
echo "Available commands:"
echo "${swupd_commands[@]}" | tr ' ' '\n'
exit 1
