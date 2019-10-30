#!/bin/bash

# shellcheck source=scripts/flag_validator_common.bash
source "$(dirname "${BASH_SOURCE[0]}")"/flag_validator_common.bash
conflict=0

# add 3rd-party swupcommands
swupd_commands+=("3rd-party add" "3rd-party remove" "3rd-party list")

usage() {
	cat <<-EOM

		Validates that the flags used in swupd vs swupd commands are not duplicated

		Usage:
		flag_validator_3rd_party.bash <command> <sub-command> <flag>

		Example:
		flag_validator_3rd_party.bash 3rd-party add w

		Notes:
			If run with no argument, it validates the existing flags
			If run with the <command> <sub-command> <flag>
			arguments, it validate that <flag> is a valid option for
			<command> <subcommand>
			If <flag> is a global option, use "global <flag>"

	EOM
}


# Entry point
arg_word_count=$(echo "$1 $2" | wc -w)

if [ "$arg_word_count" == 1 ]; then
	echo "Error: Expects a sub-command arg"
	usage
	exit 1
elif [ "$arg_word_count" == 2 ]; then
	arg="$1 $2"
fi

if [ "$arg" == "--help" ] || [ "$arg" == "-h" ]; then
	usage
	exit
fi

# If run with no argument, it validates the existing flags
if [ -z "$arg" ]; then
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
	if [ "$val" == "$arg" ]; then
		# remove the '-' if it has one
		flag=$(echo "$flag" | tr --delete '-')
		validate_flag "$arg" "$flag"
		exit "$conflict"
	fi
done

echo "Error: The provided command '$arg' is invalid"
echo "Available commands:"
echo "${swupd_commands[@]}" | tr ' ' '\n'
exit 1
