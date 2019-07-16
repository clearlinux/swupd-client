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

local context curcontext="$curcontext" state state_descr line subcmd ops ret=1
local -A opt_args
local -A val_args
local installed # cache for installed bundles

(( $+functions[_swupd_bundle_add] )) ||
  _swupd_bundle_add () {
    local bundles
    local MoM
    if [ -r /var/tmp/swupd/Manifest.MoM ]; then
      MoM=/var/tmp/swupd/Manifest.MoM
    elif [ -r /var/lib/swupd/version ] && [ -r /var/lib/swupd/"$(cat /var/lib/swupd/version)"/Manifest.MoM ]; then
      MoM=/var/lib/swupd/"$(cat /var/lib/swupd/version)"/Manifest.MoM
    fi
    if [ -n "$MoM" ]; then
      bundles="$(sed '1,/^$/d; s/^.*\t/ /; /.*\.I\..*/d' "$MoM" | LC_ALL=C sort )"
      bundles='('"$(sed 's/\n/ /g' <<< $bundles)"')'
    fi
    _alternative "$bundles" && ret=0
  }

(( $+functions[_swupd_bundle_rm] )) ||
  _swupd_bundle_rm () {
    local bundles i
    if [ -z $installed ]; then
      installed="$(unset CDPATH; test -d /usr/share/clear/bundles && \
                     find /usr/share/clear/bundles/ -maxdepth 1 -type f ! -name os-core -printf '%f ')"
    fi
    bundles=$installed
    for i in ${words:0:-1}; do
        bundles=${bundles/$i}
    done
    _alternative \
      "bundle:installed bundles: _values -s ' ' $bundles" && ret=0
  }

(( $+functions[_swupd_hashdump] )) ||
  _swupd_hashdump () {
    local dumppath dumpfile
    if [[ "$words" =~ '.* (-p +|--path +|--path=).*' ]]; then
      dumppath="$(echo $words | sed -E 's/.* +(-p|--path)//' | sed -E 's/(\=| +)//' | sed -E 's/ +-.*//')"
    fi
    if [ -n "$dumppath" ]; then
      dumpfile='('"$(ls -AFL "$dumppath" | grep -v '/$')"')'
      _alternative "$dumpfile" && ret=0
    else
      _path_files && ret=0
    fi
  }

# TODO: combine this with _swupd_bundle_rm
(( $+functions[_swupd_diagnose] )) ||
  _swupd_diagnose () {
    local bundles i
    if [ -z $installed ]; then
      installed="$(unset CDPATH; test -d /usr/share/clear/bundles && \
                     find /usr/share/clear/bundles/ -maxdepth 1 -type f ! -name os-core -printf '%f ')"
    fi
    bundles=$installed
    for i in ${words:0:-1}; do
      bundles=${bundles/$i}
    done
    _alternative \
      "bundle:installed bundles: _values -s , $bundles" && ret=0
  }

local -a global_opts; global_opts=(
  + '(help)'
  '(-)'{-h,--help}'[Show help options]'
  + '(msg)'
  '(help)--quiet[Quiet output. Print only important information and errors]'
  '(help)--debug[Print extra information to help debugging problems]'
  '(help -j --json-output)'{-j,--json-output}'[Print all output as a JSON stream]'
  + options
  '(help status -p --path)'{-p,--path=}'[Use \[PATH...\] as the path to verify (eg\: a chroot or btrfs subvol)]:path:_path_files -/'
  '(help status -u --url)'{-u,--url=}'[RFC-3986 encoded url for version string and content file downloads]:url:_urls'
  '(help status -P --port)'{-P,--port=}'[Port number to connect to at the url for version string and content file downloads]:port:_ports'
  '(help status -c --contenturl)'{-c,--contenturl=}'[RFC-3986 encoded url for content file downloads]:url:_urls'
  '(help status -v --versionurl)'{-v,--versionurl=}'[RFC-3986 encoded url for version file downloads]:url:_urls'
  '(help status -F --format)'{-F,--format=}'[\[staging,1,2,etc.\]\:the format suffix for version file downloads]:format:()'
  '(help status -n --nosigcheck)'{-n,--nosigcheck}'[Do not attempt to enforce certificate or signature checking]'
  '(help status -I --ignore-time)'{-I,--ignore-time}'[Ignore system/certificate time when validating signature]'
  '(help status -S --statedir)'{-S,--statedir=}'[Specify alternate swupd state directory]:statedir:_path_files -/'
  '(help status -C --certpath)'{-C,--certpath=}'[Specify alternate path to swupd certificates]:certpath:_path_files -/'
  '(help status -t --time)'{-t,--time}'[Show verbose time output for swupd operations]'
  '(help status -N --no-scripts)'{-N,--no-scripts}'[Do not run the post-update scripts and boot update tool]'
  '(help status -b --no-boot-update)'{-b,--no-boot-update}'[Do not install boot files to the boot partition (containers)]'
  '(help status -W --max-parallel-downloads)'{-W,--max-parallel-downloads=}'[Set the maximum number of parallel downloads]:downloadThreads:()'
  '(help status -r --max-retries)'{-r,--max-retries=}'[Maximum number of retries for download failures]:maxRetries:()'
  '(help status -d --retry-delay)'{-d,--retry-delay}'[Initial delay between download retries, this will be doubled for each retry]:retryDelay:()'
  '(help status --wait-for-scripts )--wait-for-scripts[Wait for the post-update scripts to complete]'
)

_arguments -C \
           '(-)'{-h,--help}'[Show help options]' \
           '(-)'{-v,--version}'[Output version information and exit]:version:->ops' \
           ':swupd subcommand:->subcmd' \
           '*:: :->args' \
  && ret=0

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
      )
      _describe -t subcmds 'swupd subcommand' subcmds && ret=0
      ;;
    args)
      case $line[1] in
	      info)
          _arguments $global_opts && ret=0
		      ;;
        autoupdate)
          local -a autoupdates; autoupdates=(
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
            '(help status --download )--download[Download all content, but do not actually install the update]'
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
            '(help)--skip-diskspace-check[Do not check free disk space before adding bundle]'
            '*:bundle-add:_swupd_bundle_add'
          )
          _arguments $addbundle && ret=0
          ;;
        bundle-remove)
          local -a rmbundle; rmbundle=(
            $global_opts
            '*:bundle-remove:_swupd_bundle_rm'
          )
          _arguments $rmbundle && ret=0
          ;;
        bundle-list)
          local -a lsbundle; lsbundle=(
            $global_opts
            + '(list)'
            '(help)'{-a,--all}'[List all available bundles for the current version of Clear Linux]'
            '(help)--deps=[List bundles included by BUNDLE]:deps:()'
            '(help)'{-D,--has-dep=}'[List dependency tree of all bundles which have BUNDLE as a dependency]:has-dep:()'
          )
          _arguments $lsbundle && ret=0
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
            '(help -l --library)'{-l,--library=}'[Search paths where libraries are located for a match]:library:_path_files -/'
            '(help -B --binary)'{-B,--binary=}'[Search paths where binaries are located for a match]:binary:_path_files -/'
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
        diagnose|repair)
          local -a diagnoses; diagnoses=(
            $global_opts
            '(help -B --bundles)'{-B,--bundles=}'[Ensure BUNDLES are installed correctly. Example: --bundles=os-core,vi]:diagnose:_swupd_diagnose'
            '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
            '(help -q --quick)'{-q,--quick}'[Don`t compare hashes, only fix missing files]'
            '(help -V --version)'{-V,--version=}'[Verify against manifest version V]:version:()'
            '(help -Y --picky)'{-Y,--picky}'[List files which should not exist]'
            '(help -w --picky-whitelist)'{-w,--picky-whitelist=}'[Any path completely matching the POSIX extended regular expression is ignored by --picky. Matched directories get skipped. Example\: /var|/etc/machine-id. Default\: /usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src]:picky-whitelist:_path_files -/'
            '(help -X --picky-tree)'{-X,--picky-tree=}'[Selects the sub-tree where --picky looks for extra files. Default\: /usr]:picky-tree:_path_files -/'
          )
          _arguments $diagnoses && ret=0
          ;;
        os-install)
          local -a osinstall; osinstall=(
            $global_opts
            '(help -B --bundles)'{-B,--bundles=}'[Ensure BUNDLES are installed correctly. Example: --bundles=os-core,vi]:diagnose:_swupd_diagnose'
            '(help -x --force)'{-x,--force}'[Attempt to proceed even if non-critical errors found]'
            '(help -V --version)'{-V,--version=}'[Verify against manifest version V]:version:()'
          )
          _arguments $osinstall && ret=0
          ;;
        mirror)
          local -a mirrors; mirrors=(
            $global_opts
            + '(mirror)'
            '(help)'{-s,--set}'[set mirror url]:mirro:_urls'
            '(help)'{-U,--unset}'[unset mirror url]'
          )
          _arguments $mirrors && ret=0
          ;;
        clean)
          local -a cleans; cleans=(
            $global_opts
            '--all[remove all the content including recent metadata]'
            '--dry-run[just print files that would be removed]'
          )
          _arguments $cleans && ret=0
          ;;
        hashdump)
          local -a hashdumps; hashdumps=(
            '(-)'{-h,--help}'[Show help options]'
            '(-h --help -n --no-xattrs)'{-n,--no-xattrs}'[Ignore extended attributes]'
            '(-h --help -p --path)'{-p,--path=}'[Use \[PATH...\] for leading path to filename]:dumppath:_path_files -/'
            '(-h --help)--quiet[Quiet output. Print only important information and errors]'
            '(-h --help)--debug[Print extra information to help debugging problems]'
            ':hashdump:_swupd_hashdump'
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
