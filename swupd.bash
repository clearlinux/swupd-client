#!/bin/bash
#   Software Updater - autocompletion script
#
#      Copyright © 2018 Intel Corporation.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, version 2 or later of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


#declares the completion function
_swupd()
{
    # $1 is the command being completed, $2 is the current word being expanded
    local opts IFS=$' \t\n'
    local -i i installed
    local global="--help --url --contenturl --versionurl --port --path --format --nosigcheck --ignore-time --statedir --certpath  --time --no-scripts --no-boot-update --max-parallel-downloads --max-retries --retry-delay --json-output  --allow-insecure-http --debug --quiet "
    COMPREPLY=()
    for ((i=COMP_CWORD-1;i>=0;i--))
    do case "${COMP_WORDS[$i]}" in
	    ("$1")
	    #  TODO(castulo): remove the deprecated verify command by end of July 2019
		opts="--help --version autoupdate bundle-add bundle-remove bundle-list hashdump update diagnose check-update search search-file info clean mirror os-install repair verify "
	    break;;
	    ("info")
		opts="$global "
		break;;
	    ("autoupdate")
		opts="--help --enable --disable "
		break;;
	    ("check-update")
		opts="$global "
		break;;
		("update")
		opts="$global --download --status --force --migrate --allow-mix-collisions --keepcache "
		break;;
	    ("bundle-add")
		opts="$global --skip-diskspace-check "
		break;;
	    ("bundle-remove")
		opts="$global "
		break;;
	    ("bundle-list")
		opts="$global --all --deps --has-dep "
		break;;
		("search")
		opts="--help --all --quiet --verbose "
		break;;
		("search-file")
		opts="$global --library --binary --top --csv --init --order "
		break;;
		("diagnose")
		opts="$global --version --picky --picky-tree --picky-whitelist --quick --force --bundles "
		break;;
		("repair")
		opts="$global --version --picky --picky-tree --picky-whitelist --quick --force --bundles "
		break;;
		("os-install")
		opts="$global --version --force --bundles "
		break;;
		("mirror")
		opts="$global --set --unset "
		break;;
	    ("clean")
		opts="$global --all --dry-run "
		break;;
	    ("hashdump")
		opts="--help --no-xattrs --path --debug --quiet "
		break;;
	    ("verify")
		opts="$global --version --manifest --fix --picky --picky-tree --picky-whitelist --install --quick --force --install "
		break;;
	esac
    done
    # Add in additional completion options if we need to
    if (( i >= 0 ))
    then
	case "${COMP_WORDS[$i]}" in
	    ("bundle-add")
		MoM=""
		if [ -r /var/tmp/swupd/Manifest.MoM ]
		then MoM=/var/tmp/swupd/Manifest.MoM
		elif [ -r /var/lib/swupd/version ] &&
		       installed=$(</var/lib/swupd/version) &&
		       [ -r "/var/lib/swupd/$installed/Manifest.MoM" ]
		then
		    MoM=/var/lib/swupd/$installed/Manifest.MoM
		fi
		if [ -n "$MoM" ]
		then
		    opts+="$( sed '1,/^$/d; s/^.*\t/ /; /.*\.I\..*/d' "$MoM" | LC_ALL=C sort )"
		fi ;;
	    ("bundle-remove")
		opts+=" $(unset CDPATH; test -d /usr/share/clear/bundles && \
			find /usr/share/clear/bundles/ -maxdepth 1 -type f ! -name os-core -printf '%f ')"
		;;
	    ("hashdump")
		# Add in filenames. TODO add in directory completion
		opts+=" $( compgen -f -- "$2" )"
		;;
	esac
    fi

    # Ignore SC2207 because that's the standard way to fill COMPREPLY
    # shellcheck disable=SC2207
    COMPREPLY=($(compgen -W "${opts}" -- "${2}"));
    return 0
}
if [ "${BASH_VERSINFO[0]}" -gt 4 ] || { [ "${BASH_VERSINFO[0]}" -eq 4 ] && [ "${BASH_VERSINFO[1]}" -ge 4 ]; }
then
	complete -F _swupd -o nosort swupd
else
	complete -F _swupd swupd
fi
