#!/bin/bash
#   Software Updater - autocompletion script
#
#	  Copyright © 2018 Intel Corporation.
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
_swupd() {
  local cur prev words subcmds third_subcmds opts IFS=$' \t\n'

  # Assign the following variables
  #   $cur   current command being completed, $cur==$2
  #   $prev  the previous word, $prev==$3
  #   $words the current line, where $words[0]==$1
  #   $cword the index of current word, $cword=$COMP_CWORD
  _init_completion -s || return

  subcmds=$(swupd -h | sed -n -E \
            -e '
            /Subcommands:/,${
            s/^([[:blank:]]+)([[:alnum:]][-[:alnum:]]*)(.*)/\2/p;
          }')

  if [[ ${words[1]} == "3rd-party" ]]; then
    swupd -h | grep -q 3rd-party || return 1

    third_subcmds=$(swupd 3rd-party -h | sed -n -E \
                                             -e '
                          /Subcommands:/,${
                          s/^([[:blank:]]+)([[:alnum:]][-[:alnum:]]*)(.*)/\2/p;
                        }')
    if [[ $prev == "3rd-party" ]]; then
      if [[ $cur == -* ]]; then
        opts=$(_parse_help swupd "3rd-party -h")
      else
        opts=$third_subcmds
      fi
    elif grep -q "${words[2]}" <<<"$third_subcmds"; then
      if [[ $cur == -* ]]; then
        opts=$(_parse_help swupd "3rd-party ${words[2]} -h")
      fi
    else
      return 1
    fi
  elif [[ $prev == "swupd" ]]; then
    if [[ $cur == -* ]]; then
      opts=$(_parse_help swupd)
    else
      opts="$subcmds"
    fi
  elif grep -q "${words[1]}" <<<"$subcmds"; then
    if [[ $cur == -* ]]; then
      # [[ $cur == -- ]] && printf "%s" "$prev"
      opts=$(_parse_help swupd "${words[1]} -h")
    else
      case "${words[1]}" in
        # Add additional completions here
        ("bundle-add")
          # only show the list of upstream bundles if not using "3rd-party bundle-add"
          local MoM version

          if [ -r /var/tmp/swupd/Manifest.MoM ]; then
            MoM=/var/tmp/swupd/Manifest.MoM
          else
            version=$(swupd info --quiet)
            [ -r "/var/lib/swupd/$version/Manifest.MoM" ] && MoM=/var/lib/swupd/$version/Manifest.MoM
          fi

          if [ -n "$MoM" ]; then
            opts="$( sed '/^[^M]/d' "$MoM" | cut -f4 | LC_ALL=C sort )"
          else
            return 1
          fi
          ;;
        ("bundle-remove")
          # only show the list of installed bundles if not using "3rd-party bundle-remove"
          opts="$(unset CDPATH; test -d /usr/share/clear/bundles && \
                  find /usr/share/clear/bundles/ -maxdepth 1 -type f ! -name os-core -printf '%f ')"
          ;;
        ("hashdump")
          # Add in filenames. TODO add in directory completion
          opts="$( compgen -f -- "$cur" )"
          ;;
      esac
    fi
  else
    return 1
  fi

  # Ignore SC2207 because that's the standard way to fill COMPREPLY
  # shellcheck disable=SC2207
  COMPREPLY=($(compgen -W "$opts" -- "$cur"));
  # Ignore SC2128 because only when there's one candidate it's completed on `Tab`
  # shellcheck disable=SC2128
  [[ $COMPREPLY == *= ]] && compopt -o nospace
}

complete -o nosort -F _swupd swupd
