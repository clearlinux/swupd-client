#!bin/bash

FUNC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_FILENAME=$(basename "$BATS_TEST_FILENAME")
TEST_NAME=${TEST_FILENAME%.bats}
THEME_DIRNAME="$BATS_TEST_DIRNAME"

export TEST_NAME
export THEME_DIRNAME
export FUNC_DIR
export SWUPD_DIR="$FUNC_DIR/../.."
export SWUPD="$SWUPD_DIR/swupd"

# Error codes
export EBUNDLE_MISMATCH=2  # at least one local bundle mismatches from MoM
export EBUNDLE_REMOVE=3  # cannot delete local bundle filename
export EMOM_NOTFOUND=4  # MoM cannot be loaded into memory (this could imply network issue)
export ETYPE_CHANGED_FILE_RM=5  # do_staging() couldn't delete a file which must be deleted
export EDIR_OVERWRITE=6  # do_staging() couldn't overwrite a directory
export EDOTFILE_WRITE=7  # do_staging() couldn't create a dotfile
export ERECURSE_MANIFEST=8  # error while recursing a manifest
export ELOCK_FILE=9  # cannot get the lock
export ECURL_INIT=11  # cannot initialize curl agent
export EINIT_GLOBALS=12  # cannot initialize globals
export EBUNDLE_NOT_TRACKED=13  # bundle is not tracked on the system
export EMANIFEST_LOAD=14  # cannot load manifest into memory
export EINVALID_OPTION=15  # invalid command option
export ENOSWUPDSERVER=16  # no net connection to swupd server
export EFULLDOWNLOAD=17  # full_download problem
export ENET404=404  # download 404'd
export EBUNDLE_INSTALL=18  # Cannot install bundles
export EREQUIRED_DIRS=19  # Cannot create required dirs
export ECURRENT_VERSION=20  # Cannot determine current OS version
export ESIGNATURE=21  # Cannot initialize signature verification
export EBADTIME=22  # System time is bad
export EDOWNLOADPACKS=23  # Pack download failed
export EBADCERT=24  # unable to verify server SSL certificate

# global constant
export zero_hash="0000000000000000000000000000000000000000000000000000000000000000"

generate_random_content() { 

	local bottom_range=${1:-5}
	local top_range=${2:-100}
	local range=$((top_range - bottom_range + 1))
	local number_of_lines=$((RANDOM%range+$bottom_range))
	< /dev/urandom tr -dc 'a-zA-Z0-9-_!@#$%^&*()_+{}|:<>?=' | fold -w 100 | head -n $number_of_lines

}

generate_random_name() { 

	local prefix=${1:-test-}
	local uuid
	
	# generate random 8 character alphanumeric string (lowercase only)
	uuid=$(< /dev/urandom tr -dc 'a-f0-9' | fold -w 8 | head -n 1)
	echo "$prefix$uuid"

}

print_stack() {

	echo "An error occurred"
	echo "Function stack (most recent on top):"
	for func in ${FUNCNAME[*]}; do
		if [ "$func" != "print_stack" ] && [ "$func" != "terminate" ]; then
			echo -e "\\t$func"
		fi
	done

}

terminate() {

	# since the library could be sourced and run from an interactive shell
	# not only from a script, we cannot use exit in interactive shells since it
	# would terminate the shell, so we are using parameter expansion with a fake
	# parameter "param" to force an error when running interactive shells
	local msg=$1
	local param
	case "$-" in
		*i*)	print_stack
				: "${param:?"$msg, exiting..."}" ;;
		*)		print_stack
				echo "$msg, exiting..."
				exit 1;;
	esac

}

validate_path() { 

	local path=$1
	if [ -z "$path" ] || [ ! -d "$path" ]; then
		terminate "Please provide a valid path"
	fi

}

validate_item() { 

	local vfile=$1
	if [ -z "$vfile" ] || [ ! -e "$vfile" ]; then
		terminate "Please provide a valid file"
	fi

}

validate_param() {

	local param=$1
	if [ -z "$param" ]; then
		terminate "Mandatory parameter missing"
	fi

}

# Writes to a file that is owned by root
# Parameters:
# - "-a": if set, the text will be appeneded to the file,
#         otherwise will be overwritten
# - FILE: the path to the file to write to
# - STREAM: the content to be written
write_to_protected_file() {

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    write_to_protected_file [-a] <file> <stream>

			Options:
			    -a    If used the text will be appended to the file, otherwise it will be overwritten
			EOM
		return
	fi
	local arg
	[ "$1" = "-a" ] && { arg=-a ; shift ; }
	local file=${1?Missing output file in write_to_protected_file}
	shift
	printf "$@" | sudo tee $arg "$file" >/dev/null

}

# Exports environment variables that are dependent on the test environment
# Parameters:
# - ENV_NAME: the name of the test environment
set_env_variables() {

	local env_name=$1
	local path
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_env_variables <environment_name>
			EOM
		return
	fi
	validate_path "$env_name"
	path=$(dirname "$(realpath "$env_name")")

	export SWUPD_OPTS="-S $path/$env_name/state -p $path/$env_name/target-dir -F staging -u file://$path/$env_name/web-dir -C $FUNC_DIR/Swupd_Root.pem -I"
	export SWUPD_OPTS_NO_CERT="-S $path/$env_name/state -p $path/$env_name/target-dir -F staging -u file://$path/$env_name/web-dir"
	export SWUPD_OPTS_MIRROR="-p $path/$env_name/target-dir"
	export SWUPD_OPTS_NO_FMT="-S $path/$env_name/state -p $path/$env_name/target-dir -u file://$path/$env_name/web-dir -C $FUNC_DIR/Swupd_Root.pem -I"
	export TEST_DIRNAME="$path"/"$env_name"
	export WEBDIR="$env_name"/web-dir
	export TARGETDIR="$env_name"/target-dir
	export STATEDIR="$env_name"/state

}

# Creates a directory with a hashed name in the specified path, if a directory
# already exists it returns the name
# Parameters:
# - PATH: the path where the directory will be created 
create_dir() { 
	
	local path=$1
	local hashed_name
	local directory

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_dir <path>
			EOM
		return
	fi
	validate_path "$path"
	
	# most directories have the same hash, so we only need one directory
	# in the files directory, if there is already one just return the path/name
	directory=$(find "$path"/* -type d 2> /dev/null)
	if [ ! "$directory" ]; then
		sudo mkdir "$path"/testdir
		hashed_name=$(sudo "$SWUPD" hashdump "$path"/testdir 2> /dev/null)
		sudo mv "$path"/testdir "$path"/"$hashed_name"
		# since tar is all we use, create a tar for the new dir
		create_tar "$path"/"$hashed_name"
		directory="$path"/"$hashed_name"
	fi
	echo "$directory"

}

# Generates a file with a hashed name in the specified path
# Parameters:
# - PATH: the path where the file will be created    
create_file() {
 
	local path=$1
	local hashed_name

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_file <path>
			EOM
		return
	fi
	validate_path "$path"

	generate_random_content | sudo tee "$path/testfile" > /dev/null
	hashed_name=$(sudo "$SWUPD" hashdump "$path"/testfile 2> /dev/null)
	sudo mv "$path"/testfile "$path"/"$hashed_name"
	# since tar is all we use, create a tar for the new file
	create_tar "$path"/"$hashed_name"
	echo "$path/$hashed_name"

}

# Creates a symbolic link with a hashed name to the specified file in the specified path.
# If no existing file is specified to point to, a new file will be created and pointed to
# by the link.
# If a file is provided but doesn't exist, then a dangling file will be created
# Parameters:
# - PATH: the path where the symbolic link will be created
# - FILE: the path to the file to point to
create_link() { 

	local path=$1
	local pfile=$2
	local hashed_name

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_link <path> [file_to_point_to]
			EOM
		return
	fi
	validate_path "$path"
	
	# if no file is specified, create one
	if [ -z "$pfile" ]; then
		pfile=$(create_file "$path")
	fi
	sudo ln -rs "$pfile" "$path"/testlink
	hashed_name=$(sudo "$SWUPD" hashdump "$path"/testlink 2> /dev/null)
	sudo mv "$path"/testlink "$path"/"$hashed_name"
	create_tar --skip-validation "$path"/"$hashed_name"
	echo "$path/$hashed_name"

}

# Creates a tar for the specified item in the same location
# Parameters:
# - --skip-validation: if this flag is set (as first parameter) the other parameter
#                      is not validated, so use this option carefully
# - ITEM: the relative path to the item (file, directory, link, manifest)
create_tar() {

	local path
	local item_name
	local skip_param_validation=false
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_tar [--skip-validation] <item>

			Options:
			    --skip-validation    If set, the function parameters will not be validated
			EOM
		return
	fi
	[ "$1" = "--skip-validation" ] && { skip_param_validation=true ; shift ; }
	local item=$1

	if [ "$skip_param_validation" = false ]; then
		validate_item "$item"
	fi

	path=$(dirname "$(realpath "$item")")
	item_name=$(basename "$item")
	# if the item is a directory exclude its content when taring
	if [ -d "$item" ]; then
		sudo tar -C "$path" -cf "$path"/"$item_name".tar --exclude="$item_name"/* "$item_name"
	else
		sudo tar -C "$path" -cf "$path"/"$item_name".tar "$item_name"
	fi

}

# Creates an empty manifest in the specified path
# Parameters:
# - PATH: the path where the manifest will be created
# - BUNDLE_NAME: the name of the bundle which this manifest will be for
create_manifest() {

	local path=$1
	local name=$2
	local version

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_manifest <path> <bundle_name>
			EOM
		return
	fi
	validate_path "$path"
	validate_param "$name"

	version=$(basename "$path")
	{
		printf 'MANIFEST\t1\n'
		printf 'version:\t%s\n' "$version"
		printf 'previous:\t0\n'
		printf 'filecount:\t0\n'
		printf 'timestamp:\t%s\n' "$(date +"%s")"
		printf 'contentsize:\t0\n'
		printf '\n'
	} | sudo tee "$path"/Manifest."$name" > /dev/null
	echo "$path/Manifest.$name"
	
}

# Adds the specified item to an existing bundle manifest
# Parameters:
# - --skip-validation: if this flag is set (as first parameter) the other parameters
#                      are not validated, so use this option carefully
# - MANIFEST: the relative path to the manifest file
# - ITEM: the relative path to the item (file, directory, symlink) to be added
# - PATH_IN_FS: the absolute path of the item in the target system when installed
add_to_manifest() { 

	local item_type
	local item_size
	local name
	local version
	local filecount
	local contentsize
	local linked_file
	local boot_type="."
	local file_path
	local skip_param_validation=false
	[ "$1" = "--skip-validation" ] && { skip_param_validation=true ; shift ; }
	local manifest=$1
	local item=$2
	local item_path=$3

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    add_to_manifest [--skip-validation] <manifest> <item> <item_path_in_fs>

			Options:
			    --skip-validation    If set, the validation of parameters will be skipped
			EOM
		return
	fi

	if [ "$skip_param_validation" = false ]; then
		validate_item "$manifest"
		validate_item "$item"
		validate_param "$item_path"
	fi

	item_size=$(stat -c "%s" "$item")
	name=$(basename "$item")
	version=$(basename "$(dirname "$manifest")")
	# add to filecount
	filecount=$(awk '/filecount/ { print $2}' "$manifest")
	filecount=$((filecount + 1))
	sudo sed -i "s/filecount:.*/filecount:\\t$filecount/" "$manifest"
	# add to contentsize 
	contentsize=$(awk '/contentsize/ { print $2}' "$manifest")
	contentsize=$((contentsize + item_size))
	# get the item type
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		item_type=M
		# MoM has a contentsize of 0, so don't increase this for MoM
		contentsize=0
		# files, directories and links are stored already hashed, but since
		# manifests are not stored hashed, we need to calculate the hash
		# of the manifest before adding it to the MoM
		name=$(sudo "$SWUPD" hashdump "$item" 2> /dev/null)
	elif [ -L "$item" ]; then
		item_type=L
		# when adding a link to a bundle, we need to make sure we add
		# its associated file too, unless it is a dangling link
		linked_file=$(readlink "$item")
		if [ -e "$(dirname "$item")"/"$linked_file" ]; then
			if [ ! "$(sudo cat "$manifest" | grep "$linked_file")" ]; then
				file_path="$(dirname "$item_path")"
				if [ "$file_path" = "/" ]; then
					file_path=""
				fi
				add_to_manifest "$manifest" "$(dirname "$item")"/"$linked_file" "$file_path"/"$(generate_random_name test-file-)"
			fi
		fi
	elif [ -f "$item" ]; then
		item_type=F
	elif [ -d "$item" ]; then
		item_type=D
	fi
	# if the file is in the /usr/lib/{kernel, modules} dir then it is a boot file
	if [ "$(dirname "$item_path")" = "/usr/lib/kernel" ] || [ "$(dirname "$item_path")" = "/usr/lib/modules/" ]; then
		boot_type="b"
	fi
	sudo sed -i "s/contentsize:.*/contentsize:\\t$contentsize/" "$manifest"
	# add to manifest content
	write_to_protected_file -a "$manifest" "$item_type.$boot_type.\\t$name\\t$version\\t$item_path\\n"
	# If a manifest tar already exists for that manifest, renew the manifest tar
	sudo rm -f "$manifest".tar
	create_tar "$manifest"
	# if the modified manifest is the MoM, sign it again since it has changed
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		sudo rm -f "$manifest".sig
		sign_manifest "$manifest"
	fi

}

# Adds the specified bundle dependency to an existing bundle manifest
# Parameters:
# - MANIFEST: the relative path to the manifest file
# - DEPENDENCY: the name of the bundle to be included as a dependency
add_dependency_to_manifest() {

	local manifest=$1
	local dependency=$2
	local path
	local manifest_name
	local version
	local pre_version
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    add_dependency_to_manifest <manifest> <dependency>
			EOM
		return
	fi
	validate_item "$manifest"
	validate_param "$dependency"

	path=$(dirname "$(dirname "$manifest")")
	version=$(basename "$(dirname "$manifest")")
	manifest_name=$(basename "$manifest")

	# if the provided manifest does not exist in the current version, it means
	# we need to copy it from a previous version.
	# this could happen for example if a manifest is created in one version (e.g. 10),
	# but the dependency should be added in a different, future version (e.g. 20)
	if [ ! -e "$path"/"$version"/"$manifest_name" ]; then
		pre_version="$version"
		while [ "$pre_version" -gt 0 ] && [ ! -e "$path"/"$pre_version"/"$manifest_name" ]; do
				pre_version=$(awk '/previous/ { print $2 }' "$path"/"$pre_version"/Manifest.MoM)
		done
		sudo cp "$path"/"$pre_version"/"$manifest_name" "$path"/"$version"/"$manifest_name"
		sudo sed -i "s/version:.*/version:\\t$version/" "$manifest"
		sudo sed -i "s/previous:.*/previous:\\t$pre_version/" "$manifest"
	fi
	sudo sed -i "s/timestamp:.*/timestamp:\\t$(date +"%s")/" "$manifest"
	sudo sed -i "/contentsize:.*/a includes:\\t$dependency" "$manifest"
	# If a manifest tar already exists for that manifest, renew the manifest tar
	sudo rm -f "$manifest".tar
	create_tar "$manifest"
	update_hashes_in_mom "$path"/"$version"/Manifest.MoM

}

# Removes the specified item from an existing bundle manifest
# Parameters:
# - MANIFEST: the relative path to the manifest file
# - ITEM: either the hash or filename of the item to be removed
remove_from_manifest() { 

	local manifest=$1
	local item=$2
	local filecount
	local contentsize
	local item_size
	local item_hash
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    remove_from_manifest <manifest> <item>
			EOM
		return
	fi
	validate_item "$manifest"
	validate_param "$item"

	# replace every / with \/ in item (if any)
	item="${item////\\/}"
	# decrease filecount and contentsize
	filecount=$(awk '/filecount/ { print $2}' "$manifest")
	filecount=$((filecount - 1))
	sudo sed -i "s/filecount:.*/filecount:\\t$filecount/" "$manifest"
	if [ "$(basename "$manifest")" != Manifest.MoM ]; then
		contentsize=$(awk '/contentsize/ { print $2}' "$manifest")
		item_hash=$(get_hash_from_manifest "$manifest" "$item")
		item_size=$(stat -c "%s" "$(dirname "$manifest")"/files/"$item_hash")
		contentsize=$((contentsize - item_size))
		sudo sed -i "s/contentsize:.*/contentsize:\\t$contentsize/" "$manifest"
	fi
	# remove the lines that match from the manifest
	sudo sed -i "/\\t$item$/d" "$manifest"
	sudo sed -i "/\\t$item\\t/d" "$manifest"
	# If a manifest tar already exists for that manifest, renew the manifest tar
	sudo rm -f "$manifest".tar
	create_tar "$manifest"
	# if the modified manifest is the MoM, sign it again
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		sudo rm -f "$manifest".sig
		sign_manifest "$manifest"
	else
		update_hashes_in_mom "$(dirname "$manifest")"/Manifest.MoM
	fi

}

# Recalculate the hashes of the elements in the specified MoM and updates it
# if there are changes in hashes.
# Parameters:
# - MANIFEST: the path to the MoM to be updated
update_hashes_in_mom() {

	local manifest=$1
	local path
	local bundles
	local bundle
	local bundle_old_hash
	local bundle_new_hash
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    update_hashes_in_mom <manifest>
			EOM
		return
	fi
	validate_item "$manifest"
	path=$(dirname "$manifest")

	IFS=$'\n'
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		bundles=("$(sudo cat "$manifest" | grep -x "M\.\.\..*" | awk '{ print $4 }')")
		for bundle in ${bundles[*]}; do
			# if the hash of the manifest changed, update it
			bundle_old_hash=$(get_hash_from_manifest "$manifest" "$bundle")
			bundle_new_hash=$(sudo "$SWUPD" hashdump "$path"/Manifest."$bundle" 2> /dev/null)
			if [ "$bundle_old_hash" != "$bundle_new_hash" ] && [ "$bundle_new_hash" != "$zero_hash" ]; then
				# replace old hash with new hash
				sudo sed -i "/\\t$bundle_old_hash\\t/s/\(....\\t\).*\(\\t.*\\t\)/\1$bundle_new_hash\2/g" "$manifest"
				# replace old version with new version
				sudo sed -i "/\\t$bundle_new_hash\\t/s/\(....\\t.*\\t\).*\(\\t\)/\1$(basename "$path")\2/g" "$manifest"
			fi
		done
		# re-order items on the manifest so they are in the correct order based on version
		sudo sort -t$'\t' -k3 -s -h -o "$manifest" "$manifest"
		# since the MoM has changed, sign it again and update its tar
		sudo rm -f "$manifest".sig
		sign_manifest "$manifest"
		sudo rm -f "$manifest".tar
		create_tar "$manifest"
	else
		echo "The provided manifest is not the MoM"
		return 1
	fi
	unset IFS

}

# Signs a manifest with a PEM key and generates the signed manifest in the same location
# Parameters:
# - MANIFEST: the path to the manifest to be signed
sign_manifest() {

	local manifest=$1

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    sign_manifest <manifest>
			EOM
		return
	fi
	validate_item "$manifest"

	sudo openssl smime -sign -binary -in "$manifest" \
    -signer "$FUNC_DIR"/Swupd_Root.pem \
    -inkey "$FUNC_DIR"/private.pem \
    -outform DER -out "$(dirname "$manifest")"/Manifest.MoM.sig
}

# Retrieves the hash value of a file or directory in a manifest
# Parameters:
# - MANIFEST: the manifest in which it will be looked at
# - ITEM: the dir or file to look for in the manifest
get_hash_from_manifest() {

	local manifest=$1
	local item=$2
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    get_hash_from_manifest <manifest> <name_in_fs>
			EOM
		return
	fi
	validate_item "$manifest"
	validate_param "$item"

	hash=$(sudo cat "$manifest" | grep $'\t'"$item"$ | awk '{ print $2 }')
	echo "$hash"

}

# Sets the current version of the target system to the desired version
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environmnt to act upon
# - NEW_VERSION: the version for the target to be set to
set_current_version() {

	local env_name=$1
	local new_version=$2

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		echo "$(cat <<-EOM
			Usage:
			    set_current_version <environment_name> <new_version>
			EOM
		)"
		return
	fi
	validate_path "$env_name"

	sudo sed -i "s/VERSION_ID=.*/VERSION_ID=$new_version/" "$env_name"/target-dir/usr/lib/os-release

}

# Sets the latest version on the "server" to the desired version
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environmnt to act upon
# - NEW_VERSION: the version for the target to be set to
set_latest_version() {

	local env_name=$1
	local new_version=$2
	validate_path "$env_name"

	write_to_protected_file "$env_name"/web-dir/version/formatstaging/latest "$new_version"

}

# Creates a test environment with the basic directory structure needed to
# validate the swupd client
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment, this should be typically the test name
# - VERSION: the version to use for the test environment, if not specified the default is 10
create_test_environment() { 

	local env_name=$1 
	local version=${2:-10}
	local mom
	validate_param "$env_name"
	
	# create all the files and directories needed
	# web-dir files & dirs
	sudo mkdir -p "$env_name"/web-dir/version/formatstaging
	write_to_protected_file "$env_name"/web-dir/version/formatstaging/latest "$version"
	sudo mkdir -p "$env_name"/web-dir/"$version"/files
	sudo mkdir -p "$env_name"/web-dir/"$version"/staged
	mom=$(create_manifest "$env_name"/web-dir/"$version" MoM)

	# target-dir files & dirs
	sudo mkdir -p "$env_name"/target-dir/usr/lib
	{
		printf 'NAME="Clear Linux Software for Intel Architecture"\n'
		printf 'VERSION=1\n'
		printf 'ID=clear-linux-os\n'
		printf 'VERSION_ID=%s\n' "$version"
		printf 'PRETTY_NAME="Clear Linux Software for Intel Architecture"\n'
		printf 'ANSI_COLOR="1;35"\n'
		printf 'HOME_URL="https://clearlinux.org"\n'
		printf 'SUPPORT_URL="https://clearlinux.org"\n'
		printf 'BUG_REPORT_URL="https://bugs.clearlinux.org/jira"\n'
	} | sudo tee "$env_name"/target-dir/usr/lib/os-release > /dev/null
	sudo mkdir -p "$env_name"/target-dir/usr/share/clear/bundles
	sudo mkdir -p "$env_name"/target-dir/usr/share/defaults/swupd
	printf '1' | sudo tee "$env_name"/target-dir/usr/share/defaults/swupd/format > /dev/null
	sudo mkdir -p "$env_name"/target-dir/etc

	# state files & dirs
	sudo mkdir -p "$env_name"/state/{staged,download,delta,telemetry}
	sudo chmod -R 0700 "$env_name"/state

	# export environment variables that are dependent of the test env
	set_env_variables "$env_name"

	# every environment needs to have at least the os-core bundle so this should be
	# added by default to every test environment
	create_bundle -L -n os-core -v "$version" -f /usr/bin/core "$env_name"

}

# Destroys a test environment
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment to be deleted
destroy_test_environment() { 

	local env_name=$1

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    destroy_test_environment <environment_name>
			EOM
		return
	fi
	validate_path "$env_name"

	# since the action to be performed is very destructive, at least
	# make sure the directory does look like a test environment
	for var in "state" "target-dir" "web-dir"; do
		if [ ! -d "$env_name/$var" ]; then
			echo "The name provided doesn't seem to be a valid test environment"
			return 1
		fi
	done
	sudo rm -rf "$env_name"

}

# Creates a bundle in the test environment. The bundle can contain files, directories or symlinks.
create_bundle() { 

	cb_usage() { 
		cat <<-EOM
		Usage:
		    create_bundle [-L] [-n] <bundle_name> [-v] <version> [-d] <list of dirs> [-f] <list of files> [-l] <list of links> ENV_NAME

		Options:
		    -L    When the flag is selected the bundle will be 'installed' in the target-dir, otherwise it will only be created in web-dir
		    -n    The name of the bundle to be created, if not specified a name will be autogenerated
		    -v    The version for the bundle, if non selected version 10 will be used
		    -d    Comma-separated list of directories to be included in the bundle
		    -f    Comma-separated list of files to be created and included in the bundle
		    -l    Comma-separated list of symlinks to be created and included in the bundle
		    -b    Comma-separated list of dangling (broken) symlinks to be created and included in the bundle

		Notes:
		    - if no option is selected, a minimal bundle will be created with only one directory
		    - for every symlink created a related file will be created and added to the bundle as well (except for dangling links)
		    - if the '-f' or '-l' options are used, and the directories where the files live don't exist,
		      they will be automatically created and added to the bundle for each file

		Example of usage:

		    The following command will create a bundle named 'test-bundle', which will include three directories,
		    four files, and one symlink (they will be added to the bundle's manifest), all these resources will also
		    be tarred. The manifest will be added to the MoM.

		    create_bundle -n test-bundle -f /usr/bin/test-1,/usr/bin/test-2,/etc/systemd/test-3 -l /etc/test-link my_test_env

		EOM
	}
	
	local OPTIND
	local opt
	local dir_list
	local file_list
	local link_list
	local dangling_link_list
	local version
	local bundle_name
	local env_name
	local files_path
	local version_path
	local manifest
	local local_bundle=false

	# If no parameters are received show help
	if [ $# -eq 0 ]; then
		create_bundle -h
		return
	fi
	set -f  # turn off globbing
	while getopts :v:d:f:l:b:n:L opt; do
		case "$opt" in
			d)	IFS=, read -r -a dir_list <<< "$OPTARG"  ;;
			f)	IFS=, read -r -a file_list <<< "$OPTARG" ;;
			l)	IFS=, read -r -a link_list <<< "$OPTARG" ;;
			b)	IFS=, read -r -a dangling_link_list <<< "$OPTARG" ;;
			n)	bundle_name="$OPTARG" ;;
			v)	version="$OPTARG" ;;
			L)	local_bundle=true ;;
			*)	cb_usage
				return ;;
		esac
	done
	set +f  # turn globbing back on
	env_name=${@:$OPTIND:1}

	# set default values
	bundle_name=${bundle_name:-$(generate_random_name test-bundle-)}
	version=${version:-10}
	if [ -z "$dir_list" ] && [ -z "$file_list" ] && [ -z "$link_list" ] && [ -z "$dangling_link_list" ] ; then
		# if nothing was specified to be created, at least create
		# one directory which is the bare minimum for a bundle
		dir_list=(/usr/bin)
	fi
	# all bundles should include their own tracking file, so append it to the
	# list of files to be created in the bundle
	file_list+=(/usr/share/clear/bundles/"$bundle_name")
	
	# get useful paths
	validate_path "$env_name"
	version_path="$env_name"/web-dir/"$version"
	files_path="$version_path"/files
	target_path="$env_name"/target-dir

	# 1) create the initial manifest
	manifest=$(create_manifest "$version_path" "$bundle_name")
	if [ "$DEBUG" == true ]; then
		echo "Manifest -> $manifest"
	fi
	
	# 2) Create one directory for the bundle and add it the requested
	# times to the manifest.
	# Every bundle has to have at least one directory,
	# hashes in directories vary depending on owner and permissions,
	# so one directory hash can be reused many times
	bundle_dir=$(create_dir "$files_path")
	if [ "$DEBUG" == true ]; then
		echo "Directory -> $bundle_dir"
	fi
	# Create a zero pack for the bundle and add the directory to it
	sudo tar -cf "$version_path"/pack-"$bundle_name"-from-0.tar --exclude="$bundle_dir"/*  "$bundle_dir"
	for val in "${dir_list[@]}"; do
		# the "/" directory is not allowed in the manifest
		if [ val != "/" ]; then
			add_to_manifest "$manifest" "$bundle_dir" "$val"
			if [ "$local_bundle" = true ]; then
				sudo mkdir -p "$target_path$val"
			fi
		fi
	done
	
	# 3) Create the requested file(s)
	for val in "${file_list[@]}"; do
		# if the directories the file is don't exist, add them to the bundle
		# there are 2 exceptions, the dir of the tracking file and "\"
		fdir=$(dirname "$val")
		if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ] && [ "$fdir" != "/usr/share/clear/bundles" ] && [ "$fdir" != "/" ]; then
			bundle_dir=$(create_dir "$files_path")
			add_to_manifest "$manifest" "$bundle_dir" "$fdir"
			# add each one of the directories of the path if they are not in the manifest already
			while [ "$(dirname "$fdir")" != "/" ]; do
				fdir=$(dirname "$fdir")
				if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ]; then
					add_to_manifest "$manifest" "$bundle_dir" "$fdir"
				fi
			done
		fi
		bundle_file=$(create_file "$files_path")
		if [ "$DEBUG" == true ]; then
			echo "file -> $bundle_file"
		fi
		add_to_manifest "$manifest" "$bundle_file" "$val"
		# Add the file to the zero pack of the bundle
		sudo tar -rf "$version_path"/pack-"$bundle_name"-from-0.tar "$bundle_file"
		# if the local_bundle flag is set, copy the files to the target-dir as if the
		# bundle had been locally installed
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			sudo cp "$bundle_file" "$target_path$val"
		fi 
	done
	
	# 4) Create the requested link(s) in the bundle
	for val in "${link_list[@]}"; do
		# if the directory the link is doesn't exist,
		# add it to the bundle (except if the directory is "/")
		fdir=$(dirname "$val")
		if [ "$fdir" != "/" ]; then
			if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ]; then
				bundle_dir=$(create_dir "$files_path")
				add_to_manifest "$manifest" "$bundle_dir" "$fdir"
			fi
		fi
		bundle_link=$(create_link "$files_path")
		sudo tar -rf "$version_path"/pack-"$bundle_name"-from-0.tar "$bundle_link"
		add_to_manifest "$manifest" "$bundle_link" "$val"
		# Add the file pointed by the link to the zero pack of the bundle
		pfile=$(basename "$(readlink -f "$bundle_link")")
		sudo tar -rf "$version_path"/pack-"$bundle_name"-from-0.tar "$files_path"/"$pfile"
		if [ "$DEBUG" == true ]; then
			echo "link -> $bundle_link"
			echo "file pointed to -> $(readlink -f "$bundle_link")"
		fi
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled copy the link to target-dir but also
			# copy the file it points to
			pfile_path=$(awk "/$(basename $pfile)/"'{ print $4 }' "$manifest")
			sudo cp "$files_path"/"$pfile" "$target_path$pfile_path"
			sudo ln -rs "$target_path$pfile_path" "$target_path$val"
		fi
	done
	
	# 5) Create the requested dangling link(s) in the bundle
	for val in "${dangling_link_list[@]}"; do
		# if the directory the link is doesn't exist,
		# add it to the bundle (except if the directory is "/")
		fdir=$(dirname "$val")
		if [ "$fdir" != "/" ]; then
			if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ]; then
				bundle_dir=$(create_dir "$files_path")
				add_to_manifest "$manifest" "$bundle_dir" "$fdir"
			fi
		fi
		# Create a link passing a file that does not exits
		bundle_link=$(create_link "$files_path" "$files_path"/"$(generate_random_name does_not_exist-)")
		sudo tar -rf "$version_path"/pack-"$bundle_name"-from-0.tar "$bundle_link"
		add_to_manifest --skip-validation "$manifest" "$bundle_link" "$val"
		# Add the file pointed by the link to the zero pack of the bundle
		if [ "$DEBUG" == true ]; then
			echo "dangling link -> $bundle_link"
		fi
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled since we cannot copy a bad link create a new one
			# in the appropriate location in target-dir with the corrent name
			sudo ln -s "$(generate_random_name /does_not_exist-)" "$target_path$val"
		fi
	done

	# 6) Add the bundle to the MoM
	add_to_manifest "$version_path"/Manifest.MoM "$manifest" "$bundle_name"

	# 7) Create/renew manifest tars
	sudo rm -f "$version_path"/Manifest.MoM.tar
	create_tar "$version_path"/Manifest.MoM

	# 8) Sign the manifest MoM
	sign_manifest "$version_path"/Manifest.MoM

	# 9) Create the subscription to the bundle if the local_bundle flag is enabled
	if [ "$local_bundle" = true ]; then
		sudo touch "$target_path"/usr/share/clear/bundles/"$bundle_name"
	fi

}

# Removes a bundle from the target-dir and/or the web-dir
# Parameters
# - -L: if this option is set the bundle is removed from the target-dir only,
#       otherwise it is removed from target-dir and web-dir
# - BUNDLE_MANIFEST: the manifest of the bundle to be removed
remove_bundle() {

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    remove_bundle [-L] <bundle_manifest>

			Options:
			    -L   If set, the bundle will be removed from the target-dir only,
			         otherwise it is removed from both target-dir and web-dir
			EOM
		return
	fi
	local remove_local=false
	[ "$1" = "-L" ] && { remove_local=true ; shift ; }
	local bundle_manifest=$1
	local target_path
	local version_path
	local bundle_name
	local file_names
	local dir_names
	local manifest_file

	# if the bundle's manifest is not found just return
	if [ ! -e "$bundle_manifest" ]; then
		echo "$(basename $bundle_manifest) not found, maybe the bundle was already removed"
		return
	fi

	target_path=$(dirname "$bundle_manifest" | cut -d "/" -f1)/target-dir
	version_path=$(dirname "$bundle_manifest")
	manifest_file=$(basename "$bundle_manifest")
	bundle_name=${manifest_file#Manifest.}

	# remove all files that are in the manifest from target-dir first
	file_names=($(awk '/^[FL]...\t/ { print $4 }' "$bundle_manifest"))
	for fname in ${file_names[@]}; do
		sudo rm -f "$target_path$fname"
	done
	# now remove all directories in the manifest (only if empty else they
	# may be used by another bundle)
	dir_names=($(awk '/^D...\t/ { print $4 }' "$bundle_manifest"))
	for dname in ${dir_names[@]}; do
		sudo rmdir "$target_path$dname" 2> /dev/null
	done
	if [ "$remove_local" = false ]; then
		# remove all files that are in the manifest from web-dir
		file_names=($(awk '/^[FL]...\t/ { print $2 }' "$bundle_manifest"))
		for fname in ${file_names[@]}; do
			sudo rm "$version_path"/files/"$fname"
			sudo rm "$version_path"/files/"$fname".tar
		done
		# remove zero pack
		sudo rm "$version_path"/pack-"$bundle_name"-from-0.tar
		# finally remove the manifest
		sudo rm "$version_path"/"$manifest_file"
		sudo rm "$version_path"/"$manifest_file".tar
	fi

}

# Installs a bundle in target-dir
# Parameters:
# - BUNDLE_MANIFEST: the manifest of the bundle to be installed

install_bundle() {

	local bundle_manifest=$1
	local target_path
	local file_names
	local dir_names
	local link_names
	local fhash
	local lhash
	local fdir
	local manifest_file
	local bundle_name

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    install_bundle <bundle_manifest>
			EOM
		return
	fi
	validate_item "$bundle_manifest"
	target_path=$(dirname "$bundle_manifest" | cut -d "/" -f1)/target-dir
	files_path=$(dirname "$bundle_manifest")/files
	manifest_file=$(basename "$bundle_manifest")
	bundle_name=${manifest_file#Manifest.}

	# make sure the bundle is not already installed
	if [ -e "$target_path"/usr/share/clear/bundles/"$bundle_name" ]; then
		return
	fi

	# iterate through the manifest and copy all the files in its
	# correct place, start with directories
	dir_names=($(awk '/^D...\t/ { print $4 }' "$bundle_manifest"))
	for dname in ${dir_names[@]}; do
		sudo mkdir -p "$target_path$dname"
	done
	# now files
	file_names=($(awk '/^F...\t/ { print $4 }' "$bundle_manifest"))
	for fname in ${file_names[@]}; do
		fhash=$(get_hash_from_manifest "$bundle_manifest" "$fname")
		sudo cp "$files_path"/"$fhash" "$target_path$fname"
	done
	# finally links
	link_names=($(awk '/^L...\t/ { print $4 }' "$bundle_manifest"))
	for lname in ${link_names[@]}; do
		lhash=$(get_hash_from_manifest "$bundle_manifest" "$lname")
		fhash=$(readlink "$files_path"/"$lhash")
		# is the original link dangling?
		if [[ $fhash = *"does_not_exist"* ]]; then
			sudo ln -s "$fhash" "$target_path$lname"
		else
			fname=$(awk "/$fhash/ "'{ print $4 }' "$bundle_manifest")
			sudo ln -s $(basename "$fname") "$target_path$lname"
		fi
	done

}

# Cleans up the directories in the state dir
# Parameters:
# - ENV_NAME: the name of the test environment to have the state dir cleaned up
clean_state_dir() {

	local env_name=$1
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    clean_state_dir <environment_name>
			EOM
		return
	fi
	validate_path "$env_name"

	sudo rm -rf "$env_name"/state/{staged,download,delta,telemetry}
	sudo mkdir -p "$env_name"/state/{staged,download,delta,telemetry}
	sudo chmod -R 0700 "$env_name"/state

}

# Creates a new test case based on a template
# Parameters:
# - NAME: the name (and path) of the test to be generated
generate_test() {

	local name=$1
	local path

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    generate_test <test_name>
			EOM
		return
	fi
	validate_param "$name"

	path=$(dirname "$name")/
	name=$(basename "$name")

	{
		printf '#!/usr/bin/env bats\n\n'
		printf 'load "../testlib"\n\n'
		printf 'global_setup() {\n\n'
		printf '\t# global setup\n\n'
		printf '}\n\n'
		printf 'test_setup() {\n\n'
		printf '\t# create_test_environment "$TEST_NAME"\n'
		printf '\t# create_bundle -n <bundle_name> -f <file_1>,<file_2>,<file_N> "$TEST_NAME"\n\n'
		printf '}\n\n'
		printf 'test_teardown() {\n\n'
		printf '\t# destroy_test_environment "$TEST_NAME"\n\n'
		printf '}\n\n'
		printf 'global_teardown() {\n\n'
		printf '\t# global cleanup\n\n'
		printf '}\n\n'
		printf '@test "<test description>" {\n\n'
		printf '\trun sudo sh -c "$SWUPD <swupd_command> $SWUPD_OPTS <command_options>"\n'
		printf '\t# <validations>\n\n'
		printf '}\n\n'
	} > "$path$name".bats
	# make the test script executable
	chmod +x "$path$name".bats

}

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# The section below contains test fixtures that can be used from tests to create and
# cleanup test dependencies, these functions can be overwritten in the test script.
# The intention of these is to try reducing the amount of boilerplate included in
# tests since all tests require at least the creation of a  test environment
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

setup() {

	# the first time setup runs, run the global_setup
	if [ "$BATS_TEST_NUMBER" -eq 1 ]; then
		global_setup
	fi
	# in case the env was created in global_setup set environment variables
	if [ -d "$TEST_NAME" ]; then
		set_env_variables "$TEST_NAME"
	fi
	test_setup

}

teardown() {

	test_teardown
	# if the last test just ran, run the global teardown
	if [ "$BATS_TEST_NUMBER" -eq "${#BATS_TEST_NAMES[@]}" ]; then
		global_teardown
	fi

}

global_setup() {

	# dummy value in case function is not defined
	return

}

global_teardown() {

	# dummy value in case function is not defined
	return

}

# Default test_setup
test_setup() {

	create_test_environment "$TEST_NAME"

}

# Default test_teardown
test_teardown() {

	if [ "$DEBUG_TEST" != true ]; then
		destroy_test_environment "$TEST_NAME"
	fi

}

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# The section below contains functions useful for consistent test validation and output
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sep="------------------------------------------------------------------"

print_assert_failure() {

	local message=$1
	validate_param "$message"

	echo -e "\\nAssertion Failed"
	echo -e "$message"
	echo "Command output:"
	echo "------------------------------------------------------------------"
	echo "$output"
	echo "------------------------------------------------------------------"

}

use_ignore_list() {

	local ignore_enabled=$1
	local filtered_output
	validate_param "$ignore_enabled"

	# if selected, remove things in the ignore list from the actual output
	if [ "$ignore_enabled" = true ]; then
		# always remove blank lines and lines with only dots
		filtered_output=$(echo "$output" | sed -E '/^$/d' | sed -E '/^\.+$/d')
		# now remove lines that are included in any of the ignore-lists
		# there are 3 possible ignore-lists that the function is going
		# to recognize (in order of precedence):
		# - <functional_tests_directory>/<test_theme_directory>/<test_name>.ignore-list
		# - <functional_tests_directory>/<test_theme_directory>/ignore-list
		# - <functional_tests_directory>/ignore-list
		if [ -f "$THEME_DIRNAME"/"$TEST_NAME".ignore-list ]; then
			ignore_list="$THEME_DIRNAME"/"$TEST_NAME".ignore-list
		elif [ -f "$THEME_DIRNAME"/ignore-list ]; then
			ignore_list="$THEME_DIRNAME"/ignore-list
		elif [ -f "$FUNC_DIR"/ignore-list ]; then
			ignore_list="$FUNC_DIR"/ignore-list.global
		fi
		while IFS= read -r line; do
			# if the pattern from the file has a "/" escape it first so it does
			# not confuses the sed command
			line="${line////\\/}"
			filtered_output=$(echo "$filtered_output" | sed -E "/^$line$/d")
		done < "$ignore_list"
	else
		filtered_output="$output"
	fi
	echo "$filtered_output"

}

assert_status_is() {

	local expected_status=$1
	validate_param "$expected_status"

	if [ -z "$status" ]; then
		echo "The \$status environment variable is empty."
		echo "Please make sure this assertion is used inside a BATS test after a 'run' command."
		return 1
	fi

	if [ ! "$status" -eq "$expected_status" ]; then
		print_assert_failure "Expected status: $expected_status\\nActual status: $status"
		return 1
	else
		# if the assertion was successful show the output only if the user
		# runs the test with the -t flag
		echo -e "\\nCommand output:" >&3
		echo "------------------------------------------------------------------" >&3
		echo "$output" >&3
		echo -e "------------------------------------------------------------------\\n" >&3
	fi

}

assert_status_is_not() {

	local not_expected_status=$1
	validate_param "$not_expected_status"

	if [ -z "$status" ]; then
		echo "The \$status environment variable is empty."
		echo "Please make sure this assertion is used inside a BATS test after a 'run' command."
		return 1
	fi

	if [ "$status" -eq "$not_expected_status" ]; then
		print_assert_failure "Status expected to be different than: $not_expected_status\\nActual status: $status"
		return 1
	else
		# if the assertion was successful show the output only if the user
		# runs the test with the -t flag
		echo -e "\\nCommand output:" >&3
		echo "------------------------------------------------------------------" >&3
		echo "$output" >&3
		echo -e "------------------------------------------------------------------\\n" >&3
	fi

}

assert_dir_exists() {

	local vdir=$1
	validate_param "$vdir"

	if [ ! -d "$vdir" ]; then
		print_assert_failure "Directory $vdir should exist, but it does not"
		return 1
	fi

}

assert_dir_not_exists() {

	local vdir=$1
	validate_param "$vdir"

	if [ -d "$vdir" ]; then
		print_assert_failure "Directory $vdir should not exist, but it does"
		return 1
	fi

}

assert_file_exists() {

	local vfile=$1
	validate_param "$vfile"

	if [ ! -f "$vfile" ]; then
		print_assert_failure "File $vfile should exist, but it does not"
		return 1
	fi

}

assert_file_not_exists() {

	local vfile=$1
	validate_param "$vfile"

	if [ -f "$vfile" ]; then
		print_assert_failure "File $vfile should not exist, but it does"
		return 1
	fi

}

assert_in_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ ! "$actual_output" == *"$expected_output"* ]]; then
		print_assert_failure "The following text was not found in the command output:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_not_in_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ "$actual_output" == *"$expected_output"* ]]; then
		print_assert_failure "The following text was found in the command output and should not have:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_is_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ ! "$actual_output" == "$expected_output" ]]; then
		print_assert_failure "The following text was not the command output:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_is_not_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ "$actual_output" == "$expected_output" ]]; then
		print_assert_failure "The following text was the command output and should not have:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_regex_in_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ ! "$actual_output" =~ $expected_output ]]; then
		print_assert_failure "The following text (regex) was not found in the command output:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_regex_not_in_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ "$actual_output" =~ $expected_output ]]; then
		print_assert_failure "The following text (regex) was found in the command output and should not have:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_regex_is_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ ! "$actual_output" =~ ^$expected_output$ ]]; then
		print_assert_failure "The following text (regex) was not the command output:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_regex_is_not_output() {

	local actual_output
	local ignore_switch=true
	local ignore_list
	[ "$1" = "--identical" ] && { ignore_switch=false ; shift ; }
	local expected_output=$1
	validate_param "$expected_output"

	actual_output=$(use_ignore_list "$ignore_switch")
	if [[ "$actual_output" =~ ^$expected_output$ ]]; then
		print_assert_failure "The following text (regex) was the command output and should not have:\\n$sep\\n$expected_output\\n$sep"
		echo -e "Difference:\\n$sep"
		echo "$(diff -u <(echo "$expected_output") <(echo "$actual_output"))"
		return 1
	fi

}

assert_equal() {

	local val1=$1
	local val2=$2
	validate_param "$val1"
	validate_param "$val2"

	if [ "$val1" != "$val2" ]; then
		return 1
	fi

}

assert_not_equal() {

	local val1=$1
	local val2=$2
	validate_param "$val1"
	validate_param "$val2"

	if [ "$val1" = "$val2" ]; then
		return 1
	fi

}

assert_files_equal() {

	local val1=$1
	local val2=$2
	validate_item "$val1"
	validate_item "$val2"

	diff -q "$val1" "$val2"

}

assert_files_not_equal() {

	local val1=$1
	local val2=$2
	validate_item "$val1"
	validate_item "$val2"

	if diff -q "$val1" "$val2" > /dev/null; then
		echo "Files $val1 and $val2 are equal"
		return 1
	else
		return 0
	fi

}
