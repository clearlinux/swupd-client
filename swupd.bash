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
    COMPREPLY=()
    for ((i=COMP_CWORD-1;i>=0;i--))
    do case "${COMP_WORDS[$i]}" in
	    ("$1")
		opts="--help --version autoupdate bundle-add bundle-remove bundle-list hashdump update verify check-update search info clean"
	    break;;
	    ("autoupdate")
		opts="--help --enable --disable "
		break;;
	    ("bundle-add")
		opts="--help --url --contenturl --versionurl --port --path --format --force --nosigcheck --ignore-time --statedir --certpath --time --no-scripts --no-boot-update --max-parallel-pack-downloads "
		break;;
	    ("bundle-remove")
		opts="--help --path --url --contenturl --versionurl --port --format --force --nosigcheck --ignore-time --statedir --certpath "
		break;;
	    ("bundle-list")
		opts="--help --all --url --contenturl --versionurl --path --format --nosigcheck --ignore-time --statedir --certpath --deps --has-dep "
		break;;
	    ("hashdump")
		opts="--help --no-xattrs --path "
		break;;
	    ("update")
		opts="--help --download --url --port --contenturl --versionurl --status --format --path --force --nosigcheck --ignore-time --statedir --certpath --time --no-scripts --no-boot-update --migrate --allow-mix-collisions --max-parallel-pack-downloads "
		break;;
	    ("verify")
		opts="--help --manifest --path --url --port --contenturl --versionurl --fix --picky --picky-tree --picky-whitelist --install --format --quick --force --nosigcheck --ignore-time --statedir --certpath --time --no-scripts --no-boot-update --max-parallel-pack-downloads "
		break;;
	    ("check-update")
		opts="--help --url --versionurl --port --format --force --nosigcheck --path --statedir "
		break;;
	    ("search")
		opts="--help --library --binary --scope --top --csv --display-files --init --ignore-time --url --contenturl --versionurl --port --path --format --statedir --certpath "
		break;;
	    ("info")
		opts=""
		break;;
	    ("clean")
		opts="--all --dry-run --statedir --help "
		break;;
	esac
    done
    # Add in additional completion options if we need to
    if (( $i >= 0 ))
    then
	case "${COMP_WORDS[$i]}" in
	    ("bundle-add")
		MoM=""
		if [ -r /usr/share/clear/bundles/.MoM ]
		then MoM=/usr/share/clear/bundles/.MoM
		elif [ -r /var/lib/swupd/version ] &&
		       installed=$(</var/lib/swupd/version) &&
		       [ -r /var/lib/swupd/$installed/Manifest.MoM ]
		then
		    MoM=/var/lib/swupd/$installed/Manifest.MoM
		fi
		if [ -n "$MoM" ]
		then
		    opts+="$( sed '1,/^$/d; s/^.*\t/ /' "$MoM" | LC_ALL=C sort )"
		fi ;;
	    ("bundle-remove")
		opts+=" $(unset CDPATH; test -d /usr/share/clear/bundles && \
			cd /usr/share/clear/bundles && LC_ALL=C ls | grep -v '^os-core' )"
		;;
	    ("hashdump")
		# Add in filenames. TODO add in directory completion
		opts+=" $( compgen -f -- "$2" )"
		;;
	esac
    fi

    COMPREPLY=($(compgen -W "${opts}" -- ${2}));
    return 0
}
if [ ${BASH_VERSINFO[0]} -gt 4 ] || ( [ ${BASH_VERSINFO[0]} -eq 4 ] && [ ${BASH_VERSINFO[1]} -ge 4 ] )
then
	complete -F _swupd -o nosort swupd
else
	complete -F _swupd swupd
fi
