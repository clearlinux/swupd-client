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
local context curcontext="$curcontext" ret=1
local -a line state state_descr
local -A opt_args val_args

# Custom variables
local -a installed all_bundles

# Return:
#           $installed, an array of installed bundles
#           Space-separated bundle name completion
# Options:
#      -c   completion candidates are comma-separated
#      -u   only complete one bundle
#      -r   only returns array $installed, no completion
# Usage:
#           swupd bundle-remove FOO [BAR]
#      -c   swupd diagnose|repair -b|--bundles=FOO[,BAR]
#      -u   swupd bundle-list -D|--deps=|--has-deps=FOO
(( $+functions[_swupd_installed_bundles] )) ||
  _swupd_installed_bundles () {
    # $words is a special array of `words` on current buffer, excluding `sudo` and the command name
    #   Exclude the first element of words in case some bundle names are same as certain sub-command of swupd
    #   Exclude the last element so the word under cursor won't be hidden from completion
    local -a words=(${words[1,-2]}) unique comma return_only

    zparseopts 'u=unique' 'c=comma' 'r=return_only'


    installed=(${(s: :)"$(unset CDPATH; test -d /usr/share/clear/bundles && \
                          find /usr/share/clear/bundles/ -maxdepth 1 -type f ! -name os-core -printf '%f ')"})

    (( $#comma )) && local separator="-s ,"
    if (( ! $#installed )); then
      _message -r "Can't find installed bundles."
    elif (( ! $#return_only )); then
      (( ! $#unique )) && installed=(${installed:|words})
      _alternative "bundle:installed bundles: _values $separator $installed" && ret=0
    fi
  }

# Return:
#            $all_bundles, an array of available bundles in the repo
#            Comma-separated bundle name completions
# Options:
#      -f    bundles are separated by a space, in addtion, installed bundles are filtered out
# Usage:
#            swupd os-install -b|--bundles=FOO[,BAR]
#      -f    swupd bundle-add FOO [BAR]
(( $+functions[_swupd_all_bundles] )) ||
  _swupd_all_bundles () {
    local MoM separator="-s ,"
    local -a words=(${words[2,-2]}) filter

    zparseopts 'f=filter'

    if [ -r /var/tmp/swupd/Manifest.MoM ]; then
      MoM=/var/tmp/swupd/Manifest.MoM
    elif [ -r /var/lib/swupd/version ] && [ -r /var/lib/swupd/"$(cat /var/lib/swupd/version)"/Manifest.MoM ]; then
      MoM=/var/lib/swupd/"$(cat /var/lib/swupd/version)"/Manifest.MoM
    fi

    if [ -n "$MoM" ]; then
      all_bundles=($(sed '/^[^M]/d' "$MoM" | cut -f4))
      all_bundles=(${all_bundles:|words})
      if (( $#filter )); then
        _swupd_installed_bundles -r
        all_bundles=(${all_bundles:|installed})
        unset separator
      fi
      if (( $#all_bundles )); then
        _alternative "available bundles:available bundles: _values $separator $all_bundles" && ret=0
      fi
    else
      _message -r "Can't find Manifest file."
    fi
  }

# Return:
#           File completion
# Usage:
#           swupd hashdump [-p|--path=FOO] BAR
(( $+functions[_swupd_hashdump] )) ||
  _swupd_hashdump () {
    local -a words=(${words[2,-1]}) dumppath
    # Anonymous function to get the path
    () {
      zparseopts -E -a dumppath 'p:' '-path:' '-path\=:'
    } $words
    if (( $#dumppath )); then
      # Filename expansion, path start with `~` (zsh completion system doesn't expand ~)
      eval dumppath=$dumppath
      # Convert to absolute path (_path_files function cannot understand relative path)
      dumppath=$(realpath -e "$dumppath")
      _files -W "$dumppath" && ret=0
    else
      _files && ret=0
    fi
  }

# Global options for sub-commands
local -a global_opts; global_opts=(
  + '(help)'
  '(-)'{-h,--help}'[Show help options]'
  + '(msg)'
  '(help)--quiet[Quiet output. Print only important information and errors]'
  '(help)--verbose[Enables verbosity on some of the commands]'
  '(help)--debug[Print extra information to help debugging problems]'
  '(help -j --json-output)'{-j,--json-output}'[Print all output as a JSON stream]'
  + options
  '(help status -p --path)'{-p,--path=}'[Use \[PATH\] as the path to verify (eg\: a chroot or btrfs subvol)]:path: _path_files -/'
  '(help status -u --url)'{-u,--url=}'[RFC-3986 encoded url for version string and content file downloads]:url: _urls'
  '(help status -P --port)'{-P,--port=}'[Port number to connect to at the url for version string and content file downloads]:port: _ports'
  '(help status -c --contenturl)'{-c,--contenturl=}'[RFC-3986 encoded url for content file downloads]:url: _urls'
  '(help status -v --versionurl)'{-v,--versionurl=}'[RFC-3986 encoded url for version file downloads]:url: _urls'
  '(help status -F --format)'{-F,--format=}'[\[staging,1,2,etc.\]\:the format suffix for version file downloads]:format:()'
  '(help status -S --statedir)'{-S,--statedir=}'[Specify alternate swupd state directory]:statedir: _path_files -/'
  '(help status -C --certpath)'{-C,--certpath=}'[Specify alternate path to swupd certificates]:certpath: _path_files -/'
  '(help status -W --max-parallel-downloads)'{-W,--max-parallel-downloads=}'[Set the maximum number of parallel downloads]:downloadThreads:()'
  '(help status -r --max-retries)'{-r,--max-retries=}'[Maximum number of retries for download failures]:maxRetries:()'
  '(help status -d --retry-delay)'{-d,--retry-delay}'[Initial delay between download retries, this will be doubled for each retry]:retryDelay:()'
  '(help status -n --nosigcheck)'{-n,--nosigcheck}'[Do not attempt to enforce certificate or signature checking]'
  '(help status -I --ignore-time)'{-I,--ignore-time}'[Ignore system/certificate time when validating signature]'
  '(help status -t --time)'{-t,--time}'[Show verbose time output for swupd operations]'
  '(help status -N --no-scripts)'{-N,--no-scripts}'[Do not run the post-update scripts and boot update tool]'
  '(help status -b --no-boot-update)'{-b,--no-boot-update}'[Do not install boot files to the boot partition (containers)]'
  '(help status)--no-progress[Don`t print progress report]'
  '(help status)--wait-for-scripts[Wait for the post-update scripts to complete]'
)

# Level-1 completion for sub-command and options to swupd
_arguments -C \
           '(-)'{-h,--help}'[Show help options]' \
           '(-)'{-v,--version}'[Output version information and exit]' \
           ':swupd subcommand:->subcmd' \
           '*:: :->args' \
  && ret=0

# Level-2 completion for arguments and options to sub-commands
if [[ -n "$state" ]]; then
  case "$state" in
    subcmd)
      local -a subcmds; subcmds=(
        "info:Show the version and the update URLs"
        "autoupdate:Enable/disable automatic system updates"
        "check-update:Check if a new OS version is available"
        "update:Update to latest OS version"
        "bundle-add:Install a new bundle"
        "bundle-remove:Uninstall a bundle"
        "bundle-list:List installed bundles"
        "search:Searches for the best bundle to install a binary or library"
        "search-file:Command to search files in Clear Linux bundles"
        "diagnose:Verify content for OS version"
        "repair:Repair local issues relative to server manifest (will not modify ignored files)"
        "os-install:Install Clear Linux OS to a blank partition or directory"
        "mirror:Configure mirror url for swupd content"
        "clean:Clean cached files"
        "hashdump:Dump the HMAC hash of a file"
        "3rd-party: Indicate Swupd to use user's third party repo"
      )
      _describe -t subcmds 'swupd subcommand' subcmds && ret=0
      ;;
    args)
      case $line[1] in
          info)
          _arguments $global_opts && ret=0
              ;;
          3rd-party)
          local -a thirdparty; thirdparty=(
          $global_opt
          '(help)list[List third party repo(s)]'
          '(help)remove[Remove third party repo]'
          '(help)add[Add third party repo]'
          '(help)bundle-add[Install a bundle from a third party repository]'
          '(help)bundle-list[List bundles from a third party repository]'
          _arguments $thirdparty && ret=0
          ;;
          add)
          local -a add; add=(
          $global_opt
          '(help)Positional arg: [repo-name]'
          '(help)Positional arg: [repo-url]'
          _arguments $add && ret=0
          ;;
          remove)
          local -a remove; remove=(
          $global_opt
          '(help)Positional arg: [repo-name]'
          _arguments $remove && ret=0
          ;;
          list)
          local -a list; list=(
          $global_opt
          _arguments $list && ret=0
          ;;
        autoupdate)
          local -a autoupdates; autoupdates=(
        $global_opts
            '(-)--enable[enable autoupdates]'
            '(-)--disable[disable autoupdates]'
            '(-)'{-h,--help}'[Show help options]'
          )
          _arguments $autoupdates && ret=0
          ;;
        check-update)
          _arguments $global_opts && ret=0
          ;;
        update)
          local -a updates; updates=(
            $global_opts
            '(help status)--download[Download all content, but do not actually install the update]'
            '(help status)--update-search-file-index[Update the index used by search-file to speed up searches]'
            '(help status -k --keepcache)'{-k,--keepcache}'[Do not delete the swupd state directory content after updating the system]'
            '(help status -T --migrate)'{-T,--migrate}'[Migrate to augmented upstream/mix content]'
            '(help status -a --allow-mix-collisions)'{-a,--allow-mix-collisions}'[Ignore and continue if custom user content conflicts with upstream provided content]'
            + '(version)'
            '(help status)'{-V,--version=}'[Update to version V, also accepts "latest" (default)]:version:()'
            + '(status)'
            '(help options)'{-s,--status}'[Show current OS version and latest version available on server. Equivalent to "swupd check-update"]'
          )
          _arguments $updates && ret=0
          ;;
        bundle-add)
          local -a addbundle; addbundle=(
            $global_opts
            '(help)--skip-optional[Do not install optional bundles (also-add falg in Manifests)]'
            '(help)--skip-diskspace-check[Do not check free disk space before adding bundle]'
            '*:bundle-add: _swupd_all_bundles -f'
          )
          _arguments $addbundle && ret=0
          ;;
        bundle-remove)
          local -a rmbundle; rmbundle=(
            $global_opts
            '(help -x --force)'{-x,--force}'[Removes a bundle along with all its dependent bundles]'
            '(help -R --recursive)'{-R,--recursive}'[Removes a bundle and its dependencies recursively]'
            '*:bundle-remove: _swupd_installed_bundles'
          )
          _arguments $rmbundle && ret=0
          ;;
        bundle-list)
          local -a lsbundle; lsbundle=(
            $global_opts
            + '(list)'
            '(help)'{-a,--all}'[List all available bundles for the current version of Clear Linux]'
            '(help)--deps=[List bundles included by BUNDLE]:deps: _swupd_installed_bundles -u'
            '(help)'{-D,--has-dep=}'[List dependency tree of all bundles which have BUNDLE as a dependency]:has-dep: _swupd_installed_bundles -u'
          )
          _arguments $lsbundle && ret=0
          ;;
        bundle-info)
          local -a infobundle; infobundle=(
            $global_opts
            '(help -V --version)'{-V,--version=}'[Show the bundle info for the specified version V, also accepts "latest" and "current" (default)]:version:()'
            '(help)--dependencies[Show the bundle dependencies]'
            '(help)--files[Show the files installed by this bundle]'
            '*:bundle-info: _swupd_all_bundles -f'
          )
          _arguments $infobundle && ret=0
          ;;
        search)
          local -a searches; searches=(
            '(help -v -vv --verbose -q --quiet)'{-v,-vv,--verbose}'[verbose mode]'
            '(help -a -all -q --quiet)'{-a,-all}'[show all]'
            '(-)'{-q,--quiet}'[quiet mode]'
            + '(help)'
            '(*)'{-h,--help}'[help for swupd-search]'
            ':target:()'
          )
          _arguments $searches && ret=0
          ;;
        search-file)
          local -a searchfiles; searchfiles=(
            $global_opts
            '(help -l --library)'{-l,--library=}'[Search paths where libraries are located for a match]:library: _files -W "(/usr/lib/ /usr/lib64/)" -g \*.\(a\|so\)'
            '(help -B --binary)'{-B,--binary=}'[Search paths where binaries are located for a match]:binary: _files -W /usr/bin/'
            '(help -T --top)'{-T,--top=}'[Only display the top NUM results for each bundle]:top:()'
            '(help -m --csv)'{-m,--csv}'[Output all results in CSV format (machine-readable)]'
            '(help -o --order)'{-o,--order=}'[Sort the output]:order:((alpha\:"Order alphabetically\(default\)"
                                                                       size\:"Order by bundle size \(smaller to larger\)"))'
            + '(no-search)'
            '(-)'{-i,--init}'[Download all manifests then return, no search done]'
            ':target:()'
          )
          _arguments $searchfiles && ret=0
          ;;
        diagnose)
          local -a diagnoses; diagnoses=(
            $global_opts
            '(help -V --version)'{-V,--version=}'[Verify against manifest version V]:version:()'
            '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
            '(help -q --quick)'{-q,--quick}'[Don`t compare hashes, only fix missing files]'
            '(help -Y --picky)'{-Y,--picky}'[Verify and list files which is not listed in manifests]'
            '(help -X --picky-tree)'{-X,--picky-tree=}'[Selects the sub-tree where --picky and --extra-files-only looks for extra files. Default\: /usr]:picky-tree: _path_files -/'
            '(help -w --picky-whitelist)'{-w,--picky-whitelist=}'[Directories that match the regex get skipped. Example\: /var|/etc/machine-id. Default\: /usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src]:picky-whitelist:()'
            '(help)--extra-files-only[Only list files which should not exist]'
            '(help -B --bundles)'{-B,--bundles=}'[Forces swupd to only diagnose the specified BUNDLES. Example: --bundles=os-core,vi]:bundles:()'
          )
          _arguments $diagnoses && ret=0
          ;;
         repair)
          local -a repair; repair=(
            $global_opts
            '(help -V --version)'{-V,--version=}'[Compare against version VER to repair]:version:()'
            '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
            '(help -q --quick)'{-q,--quick}'[Don`t compare hashes, only fix missing files]'
            '(help -Y --picky)'{-Y,--picky}'[Verify and remove files which is not listed in manifests]'
            '(help -w --picky-whitelist)'{-w,--picky-whitelist=}'[Directories that match the regex get skipped. Example: /var|/etc/machine-id. Default: /usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src]:picky-whitelist:()'
            '(help -X --picky-tree)'{-X,--picky-tree=}'[Selects the sub-tree where --picky and --extra-files-only looks for extra files. Default\: /usr]:picky-tree: _path_files -/'
            '(help)--extra-files-only[Only remove files which should not exist]'
            '(help -B --bundles)'{-B,--bundles=}'[Forces swupd to only repair the specified BUNDLES. Example: --bundles=os-core,vi]:bundles:()'
          )
          _arguments $repair && ret=0
          ;;
        os-install)
          local -a osinstall; osinstall=(
            $global_opts
            '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
            '(help -B --bundles)'{-B,--bundles=}'[Include the specified BUNDLES in the OS installation. Example: --bundles=os-core,vi]:diagnose: _swupd_all_bundles'
            '(help -V --version)'{-V,--version=}'[If the version to install is not the latest, it can be specified with this option]:version:()'
            '(help -s --statedir-cache)'{-s,--statedir-cache=}'[After checking for content in the statedir, check the statedir-cache before downloading it over the network]'
            '(help status)--download[Download all content, but do not actually install it]'
            '(help status)--skip-optional[Do not install optional bundles (also-add flag in Manifests)]'
          )
          _arguments $osinstall && ret=0
          ;;
        mirror)
          local -a mirrors; mirrors=(
            $global_opts
            + '(mirror)'
            '(help)'{-s,--set}'[set mirror url]:mirro: _urls'
            '(help)'{-U,--unset}'[unset mirror url]'
          )
          _arguments $mirrors && ret=0
          ;;
        clean)
          local -a cleans; cleans=(
            $global_opts
            '--all[Remove all the content including recent metadata]'
            '--dry-run[Just print files that would be removed]'
          )
          _arguments $cleans && ret=0
          ;;
        hashdump)
          local -a hashdumps; hashdumps=(
            '(-)'{-h,--help}'[Show help options]'
            '(-h --help -n --no-xattrs)'{-n,--no-xattrs}'[Ignore extended attributes]'
            '(-h --help -p --path)'{-p,--path=}'[Use \[PATH...\] for leading path to filename]:dumppath: _path_files -/'
            '(-h --help --debug)--quiet[Quiet output. Print only important information and errors]'
            '(-h --help --quiet)--debug[Print extra information to help debugging problems]'
            ':hashdump: _swupd_hashdump'
          )
          _arguments $hashdumps && ret=0
          ;;
      esac
      ;;
    *)
      ret=1
      ;;
  esac
fi

return ret
