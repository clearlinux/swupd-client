#compdef swupd
# -----------------------------------------------------------------------
#   Software Updater - autocompletion script
#
#   Author: Lucius Hu - http://github.com/lebensterben
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
# -----------------------------------------------------------------------

# Variables required by zsh completion system
local context curcontext="$curcontext"
local -a line state state_descr
local -A opt_args val_args

# Custom variables shared between sub-functions
# reutrn code
local ret=1
# use `_pick_variant` to test whether `swupd --help`, contains `3rd-party`,
#    if true, `feature_thirdparty` flag is set to `1`; and `0` otherwise
local feature_thirdparty=0
_pick_variant thirdparty=3rd-party nothirdparty --help && feature_thirdparty=1
# by default it's set but null; set to 1 for 3rd-party commands,
# this is used with ZSH parameter expansion:
#   `${thirdparty:=${FOO}}`   -- expands to $FOO when $thirdparty is null
#   `${thirdparty:+${FOO}}`   -- expands to $FOO when $thirdparty is non-null
# The drawback is when $FOO is an array with more than one elements, and $FOO is supplied to
# `_arguments`, you have to do `${thirdparty:=${FOO[1]}}` `${thirdparty:=${FOO[2]}}` ... etc.
local thirdparty
# stores names of 3rd-party repos; versions of official
local -a repos
# associative arrays of installed bundles/all available bundles for each repo,
# official repo has key "", 3rd-party repos have key equals their names
local -A installed all_bundles

# main::main_commands
local -a main_commands=(
  "info:Show the version and the update URLs"
  "autoupdate:Enable/disable automatic system updates"
  "check-update:Check if a new OS version is available"
  "update:Update to latest OS version"
  "bundle-add:Install a new bundle"
  "bundle-remove:Uninstall a bundle"
  "bundle-list:List installed bundles"
  "bundle-info: Display information about a bundle"
  "search:Searches for the best bundle to install a binary or library"
  "search-file:Command to search files in Clear Linux bundles"
  "diagnose:Verify content for OS version"
  "repair:Repair local issues relative to server manifest (will not modify ignored files)"
  "os-install:Install Clear Linux OS to a blank partition or directory"
  "mirror:Configure mirror url for swupd content"
  "clean:Clean cached files"
  "hashdump:Dump the HMAC hash of a file"
)
(( $feature_thirdparty )) && main_commands+=(
    "3rd-party:Manage third party repositories"
  )
# 3rd_party::third_party_commands
local -a third_party_commands=(
  'add:Add third party repository'
  'remove:Remove third party repository'
  'list:List third party repositories'
  'info:Shows the version of third party repositories and the update URLs'
  'check-update:Check if a new version of a third party repository is available'
  'update:Update to latest version of a third party repository'
  'bundle-add:Install a bundle from a third party repository'
  'bundle-remove:Remove a bundle from a third party repository'
  'bundle-list:List bundles from a third party repository'
  'bundle-info:Display information about a bundle in a third party repository'
  'diagnose:Verify content from a third party repository'
  'repair:Repair local issues relative to a third party repository'
  'clean:Clean cached files of a third party repository'
)
# globals::global_opts
local -a global_opts=(
  + '(help)'
  '(-)'{-h,--help}'[Show help options]'
  + '(msg)'
  '(help)--quiet[Quiet output. Print only important information and errors]'
  '(help)--verbose[Enable verbosity for commands]'
  '(help)--debug[Print extra information to help debugging problems]'
  + options
  '(help -p --path)'{-p,--path=}'[Use \[PATH\] to set the top-level directory for the swupd-managed system]:path: _files -/'
  '(help -u --url)'{-u,--url=}'[RFC-3986 encoded url for version string and content file downloads]:url: _urls'
  '(help -P --port)'{-P,--port=}'[Port number to connect to at the url for version string and content file downloads]:port: _ports'
  '(help -c --contenturl)'{-c,--contenturl=}'[RFC-3986 encoded url for content file downloads]:url: _urls'
  '(help -v --versionurl)'{-v,--versionurl=}'[RFC-3986 encoded url for version file downloads]:url: _urls'
  '(help -F --format)'{-F,--format=}'[The format suffix for version file downloads]:format: _message -r "staging,1,2,etc."'
  '(help -S --statedir)'{-S,--statedir=}'[Specify alternate path for swupd cache and data directory]:statedir: _files -/'
  '(help -K --cachedir)'{-K,--cachedir=}'[Specify alternate swupd cache directory]:cachedir: _files -/'
  '(help -Z --datadir)'{-Z,--datadir=}'[Specify alternate swupd data directory]:datadir: _files -/'
  '(help -C --certpath)'{-C,--certpath=}'[Specify alternate path to swupd certificates]:certpath: _files -/'
  '(help -W --max-parallel-downloads)'{-W,--max-parallel-downloads=}'[Set the maximum number of parallel downloads]'
  '(help -r --max-retries)'{-r,--max-retries=}'[Maximum number of retries for download failures]'
  '(help -d --retry-delay)'{-d,--retry-delay=}'[Initial delay between download retries, this will be doubled for each retry]'
  '(help -n --nosigcheck --nosigcheck-latest)'{-n,--nosigcheck}'[Do not attempt to enforce certificate or signature checking]'
  '(help -I --ignore-time)'{-I,--ignore-time}'[Ignore system/certificate time when validating signature]'
  '(help -t --time)'{-t,--time}'[Show verbose time output for swupd operations]'
  '(help -N --no-scripts)'{-N,--no-scripts}'[Do not run the post-update scripts and boot update tool]'
  '(help -b --no-boot-update)'{-b,--no-boot-update}'[Do not install boot files to the boot partition (containers)]'
  '(help -j --json-output)'{-j,--json-output}'[Print all output as a JSON stream]'
  '(help -y --yes)'{-y,--yes}'[Assume yes as answer to all prompts and run non-interactively]'
  '(help)--allow-insecure-http[Allow updates over insecure connections]'
  '(help)--no-progress[Don`t print progress report]'
  '(help)--wait-for-scripts[Wait for the post-update scripts to complete]'
  '(help)--assume=[Sets an automatic response to all prompts and run non-interactively]:assume:(yes no)'
  '(help -n --nosigcheck)--nosigcheck-latest[Do not attempt to enforce signature checking when retrieving the latest version number]'
)
# options common to all 3rd-party subcommands
# when using, `${thirdparty:+${thirdparty_opts}}` expands these options when `$thirdparty` is set
local -a thirdparty_opts=(
  '(help -R --repo)'{-R,--repo=}'[Specify the 3rd-party repository to use]:repo: _swupd_repos'
)

# these subcmds are only available to official repo
local -a autoupdate=(
  '(-)--enable[enable autoupdates]'
  '(-)--disable[disable autoupdates]'
)
local -a search=(
  '(help -v -vv --verbose -q --quiet)'{-v,-vv,--verbose}'[verbose mode]'
  '(help -a -all -q --quiet)'{-a,-all}'[show all]'
  '(-)'{-q,--quiet}'[quiet mode]'
  + '(help)'
  '(*)'{-h,--help}'[help for swupd-search]'
  ':target:()'
)
local -a search_file=(
  '(help -V --version)'{-V,--version=}'[Search for a match of the given file in the specified version VER]:version: _swupd_versions'
  '(help -l --library)'{-l,--library=}'[Search for a match of the given file in the directories where libraries are located]:library: _files -W "(/usr/lib/ /usr/lib64/)" -g \*.\(a\|so\)'
  '(help -B --binary)'{-B,--binary=}'[Search for a match of the given file in the directories where binaries are located]:binary: _files -W /usr/bin/'
  '(help -T --top)'{-T,--top=}'[Only display the top NUM results for each bundle]'
  '(help -m --csv)'{-m,--csv}'[Output all results in CSV format (machine-readable)]'
  '(help -o --order)'{-o,--order=}'[Sort the output]:order:((alpha\:"Order alphabetically\(default\)"
                                                             size\:"Order by bundle size \(smaller to larger\)"))'
  + '(no-search)'
  '(-)'{-i,--init}'[Download all manifests then return, no search done]'
  ':target:()'
)
local -a os_install=(
  '(help -V --version)'{-V,--version=}'[If the version to install is not the latest, it can be specified with this option]:version: _swupd_versions -l'
  '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
  '(help -B --bundles)'{-B,--bundles=}'[Include the specified BUNDLES in the OS installation. Example: --bundles=os-core,vi]:diagnose: _swupd_all_bundles'
  '(help -s --statedir-cache)'{-s,--statedir-cache=}'[After checking for content in the statedir, check the statedir-cache before downloading it over the network]:statedir cache: _files -/'
  '(help)--download[Download all content, but do not actually install it]'
  '(help)--skip-optional[Do not install optional bundles (also-add flag in Manifests)]'
)
local -a mirror=(
  + '(mirror)'
  '(help)'{-s,--set}'[set mirror url]:mirro: _urls'
  '(help)'{-U,--unset}'[unset mirror url]'
)
local -a hashdump=(
  '(-)'{-h,--help}'[Show help options]'
  '(-h --help -n --no-xattrs)'{-n,--no-xattrs}'[Ignore extended attributes]'
  '(-h --help -p --path)'{-p,--path=}'[Use \[PATH...\] for leading path to filename]:dumppath: _files -/'
  '(-h --help --debug)--quiet[Quiet output. Print only important information and errors]'
  '(-h --help --quiet)--debug[Print extra information to help debugging problems]'
  ':hashdump: _swupd_hashdump'
)

# these subcmds are only available to 3rd-party repo
local -a thirdparty_add=(
  '(help)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
  ':repo-name: _message -r "Name of Repo"'
  ':repo-url: _urls'
)
local -a thirdparty_remove=(
  '(help)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
  ':repo-name: _swupd_repos'
)
#local -a thirdparty_list=(
#)

# these subcmds are common to official and 3rd-party repo:
#   FOO are options for both official and 3rd-party repo,
#   FOO_official and FOO_thirdparty are options for official/3rd-party repo respecively
# to expand, use
#   `${thirdparty:=${FOO_official}}`     -- expand to ${FOO_official} when $thirdparty null
#   `${thirdparty:+${FOO_thirdparty}}`   -- expand to ${FOO_thirdparty} when $thirdparty non-null

#local -a info=(
#)

#local -a check_update=(
#)

local -a update=(
  '(help -V --version -s --status)'{-V,--version=}'[Update to version V, also accepts "latest" (default)]:version: _swupd_versions -l'
  '(-)'{-s,--status}'[Show current OS version and latest version available on server. Equivalent to "swupd check-update"]'
  '(help -s --status -k --keepcache)'{-k,--keepcache}'[Do not delete the swupd state directory content after updating the system]'
  '(help -s --status)--download[Download all content, but do not actually install the update]'
)
local update_official=(
  '(help -s --status)--update-search-file-index[Update the index used by search-file to speed up searches]'
  '(help -s --status)--3rd-party[Also update content from 3rd-party repositories]'
)

local -a bundle_add=(
  '(help)--skip-optional[Do not install optional bundles (also-add flag in Manifests)]'
  '(help)--skip-diskspace-check[Do not check free disk space before adding bundle]'
  '*:bundle-add: _swupd_all_bundles -f'
)

local -a bundle_remove=(
  '(help -x --force)'{-x,--force}'[Removes a bundle along with all the bundles that depend on it]'
  '(help -R --recursive)'{-R,--recursive}'[Removes a bundle and its dependencies recursively]'
  '(help --orphans)--orphans[Removes all orphaned bundles]'
  '*:bundle-remove: _swupd_installed_bundles'
)
local -a bundle_remove_thirdparty=(
  '(help -e --repo)'{-e,--repo=}'[Specify the 3rd-party repository to use]:repo: _swupd_repos'
)

local -a bundle_list=(
  '(help -a --all -D --has-dep --deps)'{-a,--all}'[List all available bundles for the current version of Clear Linux]'
  '(help -a --all -D --has-dep --deps --status)'{-D,--has-dep=}'[List all bundles which have BUNDLE as a dependency]:has-dep: _swupd_installed_bundles -u'
  '(help -a --all -D --has-dep --deps --status)--deps=[List bundles included by BUNDLE]:deps: _swupd_installed_bundles -u'
  '(help -D --has-dep --deps)--status[Show the installation status of the listed bundles]'
  '(help -a --all -D --has-dep --deps --status --orphans)--orphans[List orphaned bundles]'
)

local -a bundle_info=(
  '(help -V --version)'{-V,--version=}'[Show the bundle info for the specified version V (current by default), also accepts "latest"]:version: _swupd_versions -l'
  '(help)--dependencies[Show the bundle dependencies]'
  '(help)--files[Show the files installed by this bundle]'
  '*:bundle-info: _swupd_all_bundles -f'
)

local -a diagnose=(
  '(help -V --version)'{-V,--version=}'[Diagnose against manifest version V]:version: _swupd_versions'
  '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
  '(help -q --quick -Y --picky --extra-files-only)'{-q,--quick}'[Don`t check corrupt files, only find missing files]'
  '(help -B --bundles -Y --picky --extra-files-only)'{-B,--bundles=}'[Forces swupd to only diagnose the specified BUNDLES]:bundles: _swupd_installed_bundles -c'
  '(help -B --bundles -q --quick -Y --picky --extra-files-only)'{-Y,--picky}'[Also list files which should not exist]'
  '(help -X --picky-tree)'{-X,--picky-tree=}'[Selects the sub-tree where --picky and --extra-files-only looks for extra files]:picky-tree: _files -/'
  '(help -w --picky-whitelist)'{-w,--picky-whitelist=}'[Directories that match the regex get skipped during --picky. Example\: /usr/man|/usr/doc]'
  '(help -B --bundles -q --quick)--extra-files-only[Like --picky, but it only perform this task]'
)
local -a diagnose_official=(
  '(help)--file[Forces swupd to only diagnose the specified file or directory (recursively)]'
)

local -a repair=(
  '(help -V --version)'{-V,--version=}'[Compare against version VER to repair]:version: _swupd_versiosn'
  '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
  '(help -q --quick -Y --picky --extra-files-only)'{-q,--quick}'[Don`t compare hashes, only fix missing files]'
  '(help -B --bundles -Y --picky --extra-files-only)'{-B,--bundles=}'[Forces swupd to only repair the specified BUNDLES]bundles: _swupd_installed_bundles -c'
  '(help -B --bundles -Y --picky -Y --picky --extra-files-only)'{-Y,--picky}'[Also remove files which should not exists]'
  '(help -X --picky-tree)'{-X,--picky-tree=}'[Changes the path where --picky and --extra-files-only look for extra files]:picky-tree: _files -/'
  '(help -w --picky-whitelist)'{-w,--picky-whitelist=}'[Directories that match the regex get skipped during --picky. Example: /usr/man|/usr/doc]'
  '(help -B --bundles -q --quick -Y --picky)--extra-files-only[Like --picky, but it only perform this task]'
)
local -a repair_official=(
  '(help)--file[Forces swupd to only repair the specified file or directory (recursively)]'
)

local -a clean=(
  '--all[Remove all the content including recent metadata]'
  '--dry-run[Just print files that would be removed]'
)

# Return:
#             $repos, an array of names of 3rd-party repos
# Options:
#             show 3rd-party repo names as completion candidates
#      -r     only returns array $repos, no completion
# Usage:
#             swupd 3rd-party remove FOO
#             swupd 3rd-party bundle-add <-R,--repo=>FOO
#      -r     internal use only, to retrieve repo names
(( $+functions[_swupd_repos] )) ||
  _swupd_repos () {
    local -a return_only

    zparseopts 'r=return_only'

    # only compute $repos when it's not set
    (( ! $#repos )) && repos=($(grep -oP '(?<=\[)(.*)(?=\])' /opt/3rd-party/repo.ini))

    if (( ! $#repos )); then
      (( ! $#return_only )) && _message -r "No 3rd-party repo found"
    else
      (( ! $#return_only )) && _describe '3rd-party repos' repos && ret=0
    fi

    return ret
  }

# Return:
#               $versions, an array of OS/repo versions
#
# Options:
#               show cached OS/3rd-party repo versions
#       -l      also allows "latest"
# Usage:
#               swupd search-file <-V|--version=>FOO
#               swupd [3rd-party] diagnose <-V|--version=>FOO
#       -l      swupd [3rd-party] <update|bundle-info> <-V|--version=>FOO
(( $+functions[_swupd_versions] )) ||
  _swupd_versions () {
    local -a latest
    local version_path=/var/lib/swupd

    zparseopts 'l=latest'

    # fail-fast
    if [ ! -r "$version_path" ]; then
      _message -r "version number" && return ret
    fi

    if [ -z $thirdparty ]; then
      # official repo
      versions+=($(find "$version_path" -maxdepth 1 -type d -regex ".*[0-9]\'" -printf '%f '))
    else
      # 3rd-party repo
      # if -R or --repo is set, only show versions from that repo
      repos=(${opt_args[--repo=]:-${opt_args[-R]}})
      # otherwise, retrieve all repo names, and get their versions
      (( $#repos )) || _swupd_repos -r

      if (( ! $#repos )); then
        return ret
      else
        for version_path in $version_path/3rd-party/${^repos}; do
          if [ -r "$version_path" ]; then
            # -mindepth is necessary in case the repo name ends with digits
            versions+=($(find "$version_path" -mindepth 1 -maxdepth 1 -type d -regex ".*[0-9]\'" -printf '%f '))
          else
            return ret
          fi
        done
      fi
    fi
    # remove duplicates, optionally prepend "latest"
    if (( $#latest )); then
      versions=("latest" ${(@u)versions})
    else
      versions=(${(@u)versions})
    fi
    _alternative \
      "available bundles:available bundles: _values $separator 'versions' $versions" && ret=0

    return ret
  }

# Return:
#               $installed, an associative array of installed bundles, for each repo
# Options:
#               completion candidates are space-separated
#      -c       completion candidates are comma-separated
#      -r       only returns array $installed, no completion
#      -u       only complete one bundle, so no need to filter
# Usage:
#               swupd bundle-remove FOO [BAR ..]
#               swupd 3rd-party bundle-remove [-e|--repo=REPO] FOO [BAR ..]
#      -c       swupd <diagnose|repair> <-B|--bundles=>FOO[,BAR,..]
#      -c       swupd 3rd-party <diagnose|repair> [-r|--repo=REPO] <-B|--bundles=>FOO[,BAR,..]
#      -u       swupd bundle-list <-D|--deps=|--has-deps=>FOO
#      -u       swupd 3rd-party bundle-list [-r|--repo=REPO] <-D|--deps=|--has-deps=>FOO
#      -r       internal use only, to retrieve installed bundle names
(( $+functions[_swupd_installed_bundles] )) ||
  _swupd_installed_bundles () {
    # $words is a special array of `words` on current buffer, excluding `sudo` and the command name
    #   Exclude the first element of words in case some bundle names are same as certain sub-command
    #   of swupd
    #   Exclude the last element so the word under cursor won't be hidden from completion
    local -a words=(${words[2,-2]}) comma return_only unique
    local separator install_path=/usr/share/clear/bundles

    zparseopts 'c=comma' 'r=return_only' 'u=unique'

    (( $#comma  )) && separator="-s ,"

    if [ -z $thirdparty ]; then
      # official repo
      installed[""]=${(s: :)"$(unset CDPATH; test -d "$install_path" && \
      find "$install_path" -maxdepth 1 -type f ! -name os-core -printf '%f ')"}

    else
      # 3rd-party repo

      # bundle-remove uses -e|--repo=, all others use -R|--repo=
      # if -e|R or --repo is set, only show bundles from that repo
      if (( $#comma || $#return_only || $#unique )); then
        repos=(${opt_args[--repo=]:-${opt_args[-R]}})
      else
        repos=(${opt_args[--repo=]:-${opt_args[-e]}})
      fi
      (( $#repos )) || _swupd_repos -r

      if (( ! $#repos )); then
        (( $#return_only )) && _message -r "No 3rd-party repo found." && return ret
      else
        for install_path in /opt/3rd-party/bundles/${^repos}$install_path; do
          installed[$repo]=${(s: :)"$(unset CDPATH; test -d "$install_path" && \
          find "$install_path" -maxdepth 1 -type f ! -name os-core -printf '%f ')"}
        done
      fi
    fi

    if (( ! $#return_only )); then
      local -a installed=(${=installed})
      if (( ! $#installed )); then
        _message -r "Cannot find installed bundles." && return ret
      else
        (( ! $#unique )) && installed=(${installed:|words})
        (( $#installed )) && \
          _alternative \
            "bundle:installed bundles: _values $separator 'installed bundles' $installed" && ret=0
        return ret
      fi
    fi
  }

# Return:
#                   $all_bundles, an array available bundles in the repo
# Options:
#                   bundles are separated by a comma
#      -f           bundles are separated by a space, and installed bundles are filtered out
# Usage:
#                   swupd os-install <-b|--bundles=>FOO[,BAR,..]
#      -f           swupd [3rd-party] bundle-add FOO [BAR ..]
(( $+functions[_swupd_all_bundles] )) ||
  _swupd_all_bundles () {
    local -a words=(${words[2,-2]}) filter
    local repo MoM version_path=/usr/share/clear/version separator="-s ,"

    zparseopts 'f=filter'

    if (( $#filter )); then
      _swupd_installed_bundles -r
      unset separator
    fi

    if [ -z $thirdparty ]; then
      # official repo
      MoM=/var/tmp/swupd/Manifest.MoM
      if [ ! -r $MoM ]; then
        [ -r "$version_path" ] && MoM=/var/lib/swupd/"$(cat $version_path)"/Manifest.MoM
        [ ! -r "$MoM" ] && _message _r "Cannot read MoM file." && return ret
      fi
      all_bundles[""]+=${$(sed '/^[^M]/d' "$MoM" | cut -f4):|words}

      (( $#filter )) &&  all_bundles[""]=${${all_bundles[""]}:|installed}
    else
      # 3rd-party repo
      # if -R or --repo is set, only show bundles from that repo
      repos=(${opt_args[--repo=]:-${opt_args[-R]}})
      (( $#repos )) || _swupd_repos -r

      if (( ! $#repos )); then
        _message -r "No 3rd-party repo found." && return ret
      else
        for repo in $repos; do
          local version=/opt/3rd-party/bundles/$repo$version_path
          if [ -r "$version" ]; then
            MoM=/var/lib/swupd/3rd-party/$repo/"$(cat $version)"/Manifest.MoM

            if [ -r "$MoM" ]; then
              all_bundles[$repo]=${$(sed '/^[^M]/d' "$MoM" | cut -f4):|words}
              if (( $#filter )); then
                local -a installed_repo=(${=installed[$repo]})
                all_bundles[$repo]=${${all_bundles[$repo]}:|installed_repo}
              fi
            else
              _message -r "Cannot read MoM file for repo $repo." && return ret
            fi
          else
            _message -r "Cannot retrieve version for repo $repo." && return ret
          fi
        done
      fi
    fi

    (( $#all_bundles )) && \
      _alternative \
        "available bundles:available bundles: _values $separator 'bundles' $all_bundles" && ret=0
    return ret
  }

# Return:
#            File completion
# Usage:
#            swupd hashdump <-p|--path=>FOO [BAR ..]
(( $+functions[_swupd_hashdump] )) ||
  _swupd_hashdump () {
    local -a dumppath

    dumppath=(${opt_args[--path=]:-${opt_args[-p]}})
    if (( $#dumppath )); then
      # Filename expansion, path start with `~` (zsh completion system doesn't expand ~)
      eval dumppath=$dumppath
      # Convert to absolute path (_files function cannot understand relative path)
      dumppath=$(realpath -e "$dumppath")
      _files -W "$dumppath" && ret=0
    else
      _files && ret=0
    fi
    return ret
  }

# dispatcher for 3rd-party subcommands, including
# - 3rd-party-only sub-commands: add, remove, list
# - common sub-commands: see `_swupd_common_subcmds`
(( $+functions[_swupd_3rdparty] )) ||
  _swupd_3rdparty () {
    _arguments -C \
               '(-)'{-h,--help}'[Show help options]' \
               '::3rd-party commands: _describe "3rd-party commands" third_party_commands' \
               '*:: :->3rdparty-args' \
      && ret=0
    [[ -z "$state" ]] && return ret
    case "$state" in
      3rdparty-args)
        case $line[1] in
          add)
            _arguments $global_opts $thirdparty_add && ret=0
            ;;
          remove)
            _arguments $global_opts $thirdparty_remove && ret=0
            ;;
          list)
            _arguments $global_opts && ret=0
            ;;
          *)
            _swupd_common_subcmds && ret=0
            ;;
        esac
        ;;
      *)
        ret=1
        ;;
    esac
  }

# dispatcher for
#            check-update, update, bundle-add, bundle-remove, bundle-list, bundle-info, diagnose,
#            repair, clean
(( $+functions[_swupd_common_subcmds] )) ||
  _swupd_common_subcmds () {
    case $line[1] in
      info)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
          && ret=0
        ;;
      check-update)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
          && ret=0
        ;;
      update)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $update \
                   ${thirdparty:=${update_official[1]}} \
                   ${thirdparty:=${update_official[2]}} \
          && ret=0
        ;;
      bundle-add)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $bundle_add \
          && ret=0
        ;;
      bundle-remove)
        # the only exception here, due to different option {-e, --repo} instead of {-R, --repo}
        _arguments $global_opts \
                   $bundle_remove \
                   ${thirdparty:+${bundle_remove_thirdparty}} \
          && ret=0
        ;;
      bundle-list)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $bundle_list \
          && ret=0
        ;;
      bundle-info)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $bundle_info \
          && ret=0
        ;;
      diagnose)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $diagnose \
                   ${thirdparty:=${diagnose_official}} \
          && ret=0
        ;;
      repair)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $repair \
                   ${thirdparty:=${repair_official}} \
          && ret=0
        ;;
      clean)
        _arguments $global_opts \
                   ${thirdparty:+${thirdparty_opts}} \
                   $clean \
          && ret=0
        ;;
      *)
        ret=1
        ;;
    esac
  }

# Entry point of the actual completion function
# Level-1 completion for sub-command and options to swupd
_arguments -C \
           '(-)'{-h,--help}'[Show help options]' \
           '(-)'{-v,--version}'[Output version information and exit]' \
           ':: : _describe "main_commands" main_commands' \
           '*:: :->args' \
  && ret=0

# Level-2 completion for arguments and options to sub-commands
[[ -z "$state" ]] && return ret
case "$state" in
  args)
    case $line[1] in
      3rd-party)
        # it shouldn't get here if `3rd-party` feature is not supported
        if (( ! $feature_thirdparty )); then
          _message -r "3rd-party is not supported." && ret=1
        else
          thirdparty=1
          _swupd_3rdparty && ret=0
        fi
        ;;
      autoupdate)
        _arguments $global_opts $autoupdate && ret=0
        ;;
      search)
        _arguments $search && ret=0
        ;;
      search-file)
        _arguments $global_opts $search_file && ret=0
        ;;
      os-install)
        _arguments $global_opts $os_install && ret=0
        ;;
      mirror)
        _arguments $global_opts $mirror && ret=0
        ;;
      hashdump)
        _arguments $hashdump && ret=0
        ;;
      *)
        _swupd_common_subcmds && ret=0
        ;;
    esac
    ;;
  *)
    ret=1
    ;;
esac

return ret
