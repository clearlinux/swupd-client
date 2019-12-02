#!/usr/bin/bash

FUNC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_ROOT_DIR="$(pwd)"
TEST_FILENAME=$(basename "$BATS_TEST_FILENAME")
TEST_NAME=${TEST_FILENAME%.bats}
THEME_DIRNAME="$BATS_TEST_DIRNAME"

export TEST_NAME
export TEST_NAME_SHORT="$TEST_NAME"
export THEME_DIRNAME
export FUNC_DIR
export SWUPD_DIR="$FUNC_DIR/../.."

# detect where the swupd binary is
if [ -e "$SWUPD" ]; then
	# nothing to be done, variable already set up
	export SWUPD
elif [ -e "$SWUPD_DIR"/swupd ]; then
	# using the path relative to the test dir
	export SWUPD="$SWUPD_DIR"/swupd
elif [ -e "$(pwd)"/swupd ]; then
	# using the current path
	SWUPD="$(pwd)"/swupd
	export SWUPD
fi

# Exit codes
export SWUPD_OK=0
export SWUPD_NO=1
export SWUPD_REQUIRED_BUNDLE_ERROR=2  # a required bundle is missing or was attempted to be removed
export SWUPD_INVALID_BUNDLE=3  # the specified bundle is invalid
export SWUPD_COULDNT_LOAD_MOM=4  # MoM cannot be loaded into memory (this could imply network issue)
export SWUPD_COULDNT_REMOVE_FILE=5  # couldn't delete a file which must be deleted
export SWUPD_COULDNT_RENAME_DIR=6  # couldn't rename a directory
export SWUPD_COULDNT_CREATE_FILE=7  # couldn't create a file
export SWUPD_RECURSE_MANIFEST=8  # error while recursing a manifest
export SWUPD_LOCK_FILE_FAILED=9  # cannot get the lock
export SWUPD_COULDNT_RENAME_FILE=10  # couldn't rename a file
export SWUPD_INIT_GLOBALS_FAILED=12  # cannot initialize globals
export SWUPD_BUNDLE_NOT_TRACKED=13  # bundle is not tracked on the system
export SWUPD_COULDNT_LOAD_MANIFEST=14  # cannot load manifest into memory
export SWUPD_INVALID_OPTION=15  # invalid command option
export SWUPD_SERVER_CONNECTION_ERROR=16  # no net connection to swupd server
export SWUPD_COULDNT_DOWNLOAD_FILE=17  # file download problem
export SWUPD_COULDNT_UNTAR_FILE=18  # couldn't untar a file
export SWUPD_COULDNT_CREATE_DIR=19  # Cannot create required dir
export SWUPD_CURRENT_VERSION_UNKNOWN=20  # Cannot determine current OS version
export SWUPD_SIGNATURE_VERIFICATION_FAILED=21  # Cannot initialize signature verification
export SWUPD_BAD_TIME=22  # System time is bad
export SWUPD_COULDNT_DOWNLOAD_PACK=23  # Pack download failed
export SWUPD_BAD_CERT=24  # unable to verify server SSL certificate
export SWUPD_DISK_SPACE_ERROR=25  # not enough disk space left (or it cannot be determined)
export SWUPD_PATH_NOT_IN_MANIFEST=26  # the required path is not in any manifest
export SWUPD_UNEXPECTED_CONDITION=27  # an unexpected condition was found
export SWUPD_SUBPROCESS_ERROR=28  # failure to execute another program in a subprocess
export SWUPD_COULDNT_LIST_DIR=29  # couldn't list the content of a directory
export SWUPD_COMPUTE_HASH_ERROR=30  # there was an error computing the hash of the specified file
export SWUPD_TIME_UNKNOWN=31  # couldn't get current system time
export SWUPD_COULDNT_WRITE_FILE=32  # couldn't write to a file
export SWUPD_MIX_COLLISIONS=33  # collisions were found between mix and upstream
export SWUPD_OUT_OF_MEMORY_ERROR=34  # swupd ran out of memory
export SWUPD_VERIFY_FAILED=35  # verify could not fix/replace/delete one or more files
export SWUPD_INVALID_BINARY=36  # binary to be executed is missing or invalid
export SWUPD_INVALID_REPOSITORY=37  # the specified 3rd-party repository is invalid

# global constant
export zero_hash="0000000000000000000000000000000000000000000000000000000000000000"

generate_random_content() { # swupd_function

	local bottom_range=${1:-5}
	local top_range=${2:-100}
	local range=$((top_range - bottom_range + 1))
	local number_of_lines=$((RANDOM%range + bottom_range))
	local total_bytes=$((number_of_lines * 400))
	< /dev/urandom head -c "$total_bytes" | tr -dc 'a-zA-Z0-9-_!@#$%^&*()_+{}|:<>?=' | fold -w 100 | head -n $number_of_lines

}

generate_random_name() { # swupd_function

	local prefix=${1:-test-}
	local uuid

	# generate random 8 character alphanumeric string (lowercase only)
	uuid=$(< /dev/urandom head -c 100 | tr -dc 'a-f0-9' | fold -w 8 | head -n 1)
	echo "$prefix$uuid"

}

# Creates public and private key
# Parameters:
# - key_path: path to private key
# - cert_path: path to public key
generate_certificate() { # swupd_function

	local key_path=$1
	local cert_path=$2
	local config=$3

	# If incorrect number of parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    generate_certificate <key_path> <cert_path>
			EOM
		return
	fi
	validate_param "$key_path"
	validate_param "$cert_path"

	debug_msg "Generating test certificate"
	# generate self-signed public and private key
	if [ -z "$config" ]; then
		sudo openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 \
			-keyout "$key_path" -out "$cert_path" \
			-subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost"
	else
		validate_item "$config"
		openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 \
			-keyout "$key_path" -out "$cert_path" \
			-subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost" \
			-config "$config"
	fi

	debug_msg "Generated private key: $key_path"
	debug_msg "Generated public key: $cert_path"
	debug_msg "Keys generated succssfully"

}

# Creates the trusted certificate store and adds one certificate
# Parameters:
# - cert_path: path of certificate to add to trust store
create_trusted_cacert() { # swupd_function

	local cert_path=$1
	local counter=0
	local subj_hash

	# If incorrect number of parameters are received show usage
	if [ $# -ne 1 ]; then
		cat <<-EOM
			Usage:
			    create_trusted_cacert <certificate_path>
			EOM
		return
	fi

	# only one test can use the trusted certificate store at the same time to
	# avoid certificate contamination from other tests and race conditions.
	until mkdir "$CACERT_DIR"; do
		sleep 1
		if [ "$counter" -eq "1000" ]; then
			echo "Timeout: creating trusted certificate"
			return 1
		fi
		counter=$((counter + 1))
	done

	# only allow the test that create CACERT_DIR to erase it
	echo '1' > "$CACERT_DIR/$TEST_NAME.lock"

	# the public key names in the CACERT_DIR must use the following format to
	# be included by swupd in the trust store: <subject hash>.<number>
	subj_hash=$(openssl x509 -subject_hash -noout -in "$cert_path")
	ln -s "$cert_path" "$CACERT_DIR/$subj_hash.0"

}

# Deletes the trusted certificate store
destroy_trusted_cacert() { # swupd_function

	# only the test that created CACERT_DIR can erase it which stops
	# tests from erasing CACERT_DIR when they fail before creating it
	if [ -f "$CACERT_DIR/$TEST_NAME.lock" ]; then
		rm -rf "$CACERT_DIR"
	fi

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

# when printing a message in a bats test, it is necessary
# to use file descriptor 3, this wrapper function makes it
# easier for users of the test library to print to screen
print() { # swupd_function

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Prints a message from a test, the message will be displayed only when
			running the test using the bats -t flag.

			Usage:
			    print <msg>
			EOM
		return
	fi

	local msg=$1
	# if file descriptor 3 is not available (for example when sourcing the library
	# from the command line) use std output instead
	if { true >&3; } 2> /dev/null; then
		echo -e "$msg" >&3
	else
		echo -e "$msg"
	fi

}

# sometimes it is useful to have extra information to debug
# a test but you don't want to print all this extra info normally,
# all messages printed with debug_msg will only be shown when the
# user sets the environment variable DEBUG_TEST=true
debug_msg() { #swupd_function

	if [ $# -eq 0 ]; then
		cat <<-EOM
			Prints a debug message from a test, the message will be displayed only
			when running the test using the bats -t flag and when the env variable
			DEBUG_TEST is set to true.

			Usage:
			    debug_msg <msg>
			EOM
		return
	fi

	local msg=$1
	if [ "$DEBUG_TEST" = true ]; then
		if [[ $msg == '\n'* ]]; then
			print ""
		fi
		print "(${FUNCNAME[1]}:${BASH_LINENO[0]}) DEBUG: ${msg/'\n'}"
		if [[ $msg == *'\n' ]]; then
			print ""
		fi
	fi

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
	if sudo sh -c "[ -z $vfile ] || [ ! -e $vfile ]"; then
		terminate "Please provide a valid file"
	fi

}

validate_param() {

	local param=$1
	if [ -z "$param" ]; then
		terminate "Mandatory parameter missing"
	fi

}

validate_number() {

	local param=$1
	if ! [[ $param =~ ^[0-9]+([.][0-9]+)?$ ]]; then
		terminate "Bad parameter provided, expecting a number"
	fi

}

wait_for_deletion() {

	local param=$1
	local timeout=5  # arbitrary timeout
	local wait_milisecs=0

	until [ $((wait_milisecs / 1000)) -eq "$timeout" ] || [ ! -e "$param" ]; do
		sleep 0.1
		wait_milisecs=$((wait_milisecs + 100))
	done

}

# Gets the latest version that exists in the environment
get_latest_version() {

	local env_name=$1

	# shellcheck disable=SC2010
	# SC2010: Don't use ls | grep. Use a glob.
	# Exception: ls has sorting options that are tricky
	# to get right with other commands.
	ls "$env_name"/web-dir | grep -E '^[0-9]+$' | sort -rn | head -n1

}

# Gets the current version of the test environment
get_system_version() {

	local env_name=$1

	awk -F = '/VERSION_ID/{ print $NF }' "$env_name"/testfs/target-dir/usr/lib/os-release

}

# Gets the previous version where a bundle manifest is found
get_manifest_previous_version() {

	local env_name=$1
	local bundle=$2
	local init_version=$3
	local prev_version

	if [ -z "$init_version" ]; then
		init_version=$(get_latest_version "$env_name")
	fi
	if [ ! -e "$env_name"/web-dir/"$init_version" ]; then
		terminate "The initial version provided '$init_version' does not exist."
	fi

	# if a manifest for the specified bundle exist in the initial version,
	# use the manifest to find its previous manifest
	if [ -e "$env_name"/web-dir/"$init_version"/Manifest."$bundle" ]; then
		awk '/^previous/ { print $2 }' "$env_name"/web-dir/"$init_version"/Manifest."$bundle"
		return
	fi

	# look backwards through versions strating from init_version
	# until we find the previous version of the bundle manifest
	prev_version="$init_version"
	while [ "$prev_version" -gt 0 ] && [ ! -e "$env_name"/web-dir/"$prev_version"/Manifest."$bundle" ]; do
		prev_version=$(awk '/^previous/ { print $2 }' "$env_name"/web-dir/"$prev_version"/Manifest.MoM)
	done

	echo "$prev_version"

}

# Copies a manifest from one version to another in a test environment
copy_manifest() {

	local env_name=$1
	local bundle=$2
	local from_version=$3
	local to_version=$4
	local from_path
	local to_path
	local manifest
	local files
	local bundle_file
	local file_tar
	local format

	from_path="$env_name"/web-dir/"$from_version"
	to_path="$env_name"/web-dir/"$to_version"

	if [ ! -e "$to_path"/Manifest."$bundle" ]; then
		sudo cp "$from_path"/Manifest."$bundle" "$to_path"/Manifest."$bundle"
	fi

	# update the manifest's header
	format=$(cat "$to_path"/format)
	update_manifest -p "$to_path"/Manifest."$bundle" version "$to_version"
	update_manifest -p "$to_path"/Manifest."$bundle" previous "$from_version"
	update_manifest "$to_path"/Manifest."$bundle" format "$format"

	# untar the zero pack into the files directory in the to_version
	sudo tar -xf "$to_path"/pack-"$bundle"-from-0.tar --strip-components 1 --directory "$to_path"/files

	# copy the tar file of each fullfile from a previous version
	files=("$(ls -I "*.tar" "$to_path"/files)")
	for bundle_file in ${files[*]}; do
		if [ ! -e "$to_path"/files/"$bundle_file".tar ]; then
			# find the existing tar in previous versions and copy
			# it to the current directory
			file_tar=$(sudo find "$env_name"/web-dir -name "$bundle_file".tar | head -n 1)
			sudo cp "$file_tar" "$to_path"/files/"$bundle_file".tar
		fi
	done

}

# Writes to a file that is owned by root
# Parameters:
# - "-a": if set, the text will be appeneded to the file,
#         otherwise will be overwritten
# - FILE: the path to the file to write to
# - STREAM: the content to be written
write_to_protected_file() { # swupd_function

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
	printf '%b' "$@" | sudo tee $arg "$file" >/dev/null

}

# Exports environment variables that are dependent on the test environment
# Parameters:
# - ENV_NAME: the name of the test environment
set_env_variables() { # swupd_function

	local env_name=$1
	local path
	local testfs_path
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
	testfs_path="$path"/"$env_name"/testfs

	debug_msg "Exporting environment variables for $env_name"
	export TEST_DIRNAME="$path"/"$env_name"
	debug_msg "TEST_DIRNAME: $TEST_DIRNAME"
	export WEBDIR="$env_name"/web-dir
	debug_msg "WEBDIR: $WEBDIR"
	export TARGETDIR="$env_name"/testfs/target-dir
	debug_msg "TARGETDIR: $TARGETDIR"
	export STATEDIR="$env_name"/testfs/state
	debug_msg "STATEDIR: $STATEDIR"
	export PATH_PREFIX="$TEST_DIRNAME"/testfs/target-dir
	debug_msg "PATH_PREFIX: $PATH_PREFIX"

	# different options for swupd
	export SWUPD_OPTS="-S $testfs_path/state -p $testfs_path/target-dir -F staging -C $TEST_ROOT_DIR/Swupd_Root.pem -I --no-progress"
	export SWUPD_OPTS_PROGRESS="-S $testfs_path/state -p $testfs_path/target-dir -F staging -C $TEST_ROOT_DIR/Swupd_Root.pem -I"
	export SWUPD_OPTS_KEEPCACHE="$SWUPD_OPTS --keepcache"
	export SWUPD_OPTS_NO_CERT="-S $testfs_path/state -p $testfs_path/target-dir -F staging -I --no-progress"
	export SWUPD_OPTS_NO_FMT="-S $testfs_path/state -p $testfs_path/target-dir -C $TEST_ROOT_DIR/Swupd_Root.pem -I --no-progress"
	export SWUPD_OPTS_NO_FMT_NO_CERT="-S $testfs_path/state -p $testfs_path/target-dir -I --no-progress"
	export SWUPD_OPTS_NO_PATH="-S $testfs_path/state -F staging -C $TEST_ROOT_DIR/Swupd_Root.pem -I --no-progress"

	export CLIENT_CERT_DIR="$testfs_path/target-dir/etc/swupd"
	export CLIENT_CERT="$CLIENT_CERT_DIR/client.pem"
	export CACERT_DIR="$SWUPD_DIR/swupd_test_certificates" # trusted key store path
	export PORT_FILE="$path/$env_name/port_file.txt" # stores web server port
	export SERVER_PID_FILE="$path/$env_name/pid_file.txt" # stores web server pid

	# Add environment variables for PORT and SERVER_PID when web server used
	if [ -f "$PORT_FILE" ]; then
		PORT=$(cat "$PORT_FILE")
		export PORT
	fi

	if [ -f "$SERVER_PID_FILE" ]; then
		SERVER_PID=$(cat "$SERVER_PID_FILE")
		export SERVER_PID
	fi

}

# Creates a directory with a hashed name in the specified path, if a directory
# already exists it returns the name
# Parameters:
# - PATH: the path where the directory will be created
create_dir() { # swupd_function

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
		hashed_name=$(sudo "$SWUPD" hashdump --quiet "$path"/testdir)
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
# - SIZE: the size of the file (in bytes), if nothing is specified the size
#         will be random but fairly small
create_file() { # swupd_function

	local path=$1
	local size=$2
	local hashed_name

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_file <path> [size in bytes]
			EOM
		return
	fi
	validate_path "$path"

	if [ -n "$size" ]; then
		< /dev/urandom head -c "$size" | sudo tee "$path/testfile" > /dev/null
	else
		generate_random_content | sudo tee "$path/testfile" > /dev/null
	fi
	hashed_name=$(sudo "$SWUPD" hashdump --quiet "$path"/testfile)
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
create_link() { # swupd_function

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
	hashed_name=$(sudo "$SWUPD" hashdump --quiet "$path"/testlink)
	sudo mv "$path"/testlink "$path"/"$hashed_name"
	create_tar --skip-validation "$path"/"$hashed_name"
	echo "$path/$hashed_name"

}

# Creates a tar for the specified item in the same location
# Parameters:
# - --skip-validation: if this flag is set (as first parameter) the other parameter
#                      is not validated, so use this option carefully
# - ITEM: the relative path to the item (file, directory, link, manifest)
create_tar() { # swupd_function

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

# Sets an existing version as minversion.
# Parameters:
# - MINVERSION_PATH: the path of the version to be set as minversion
set_as_minversion() { # swupd_function

	local minversion_path=$1
	local webdir_path
	local minversion
	local previous_version
	local bundles
	local bundle
	local mom
	local manifest
	local files
	local bundle_file

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_as_minversion <version_path>
			EOM
		return
	fi
	validate_path "$minversion_path"

	minversion="$(basename "$minversion_path")"
	webdir_path="$(dirname "$minversion_path")"

	# copy all manifests in MoM to the minversion
	mom="$minversion_path"/Manifest.MoM

	mapfile -t bundles < <(awk '/^M\.\.\./ { print $4 }' "$mom")
	IFS=$'\n'
	for bundle in ${bundles[*]}; do

		# search for the latest manifest version for the bundle
		manifest="$minversion_path"/Manifest."$bundle"
		if [ ! -e "$manifest" ]; then
			previous_version="$minversion"
			while [ "$previous_version" -gt 0 ] && [ ! -e "$manifest" ]; do
				previous_version="$(awk '/previous/ { print $2 }' "$mom")"
				manifest="$webdir_path"/"$previous_version"/Manifest."$bundle"
				mom="$webdir_path"/"$previous_version"/Manifest.MoM
			done
			sudo cp "$manifest" "$webdir_path"/"$minversion"/Manifest."$bundle"
			manifest="$webdir_path"/"$minversion"/Manifest."$bundle"
			mom="$webdir_path"/"$minversion"/Manifest.MoM
			# copy the zero pack also from the old version to the new one
			sudo cp "$webdir_path"/"$previous_version"/pack-"$bundle"-from-0.tar "$webdir_path"/"$minversion"/pack-"$bundle"-from-0.tar
			# extract the content of the zero pack in the files directory so we have
			# all files available to create future delta packs
			sudo tar -xf "$webdir_path"/"$minversion"/pack-"$bundle"-from-0.tar --strip-components 1 --directory "$webdir_path"/"$minversion"/files
			files=("$(ls -I "*.tar" "$webdir_path"/"$minversion"/files)")
			for bundle_file in ${files[*]}; do
				if [ ! -e "$webdir_path"/"$minversion"/files/"$bundle_file".tar ]; then
					create_tar "$webdir_path"/"$minversion"/files/"$bundle_file"
				fi
			done
			update_manifest -p "$manifest" version "$minversion"
			update_manifest -p "$manifest" previous "$previous_version"
		fi

		# update manifest content with latest version
		sudo sed -i "/....\\t.*\\t.*\\t.*$/s/\\(....\\t.*\\t\\).*\\(\\t\\)/\\1$minversion\\2/g" "$manifest"
		sudo rm -rf "$manifest".tar
		create_tar "$manifest"

	done
	unset IFS

	# update the MoM
	update_manifest "$mom" minversion "$minversion"
	update_hashes_in_mom "$mom"

}

# Creates an empty manifest in the specified path
# Parameters:
# - PATH: the path where the manifest will be created
# - BUNDLE_NAME: the name of the bundle which this manifest will be for
# - FORMAT: the format of the manifest
create_manifest() { # swupd_function

	local path=$1
	local name=$2
	local format=${3:-1}
	local minversion="0"
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
		printf 'MANIFEST\t%s\n' "$format"
		printf 'version:\t%s\n' "$version"

		if [ "$name" = "MoM" ]; then
			printf 'minversion:\t%s\n' "$minversion"
		fi

		printf 'previous:\t0\n'
		printf 'filecount:\t0\n'
		printf 'timestamp:\t%s\n' "$(date +"%s")"
		printf 'contentsize:\t0\n'
		printf '\n'
	} | sudo tee "$path"/Manifest."$name" > /dev/null
	echo "$path/Manifest.$name"

}

# Re-creates a manifest's tar, updates the hashes in the MoM and signs it
# Parameters:
# - MANIFEST: the manifest file to have its tar re-created
retar_manifest() { # swupd_function

	local manifest=$1
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    retar_manifest <manifest>
			EOM
		return
	fi
	validate_item "$manifest"

	sudo rm -f "$manifest".tar
	create_tar "$manifest"
	# if the modified manifest is the MoM, sign it again
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		sudo rm -f "$manifest".sig
		sign_manifest "$manifest"
	else
		# update hashes in MoM, re-creates tar and re-signs MoM
		update_hashes_in_mom "$(dirname "$manifest")"/Manifest.MoM
	fi

}

# Adds the specified item to an existing bundle manifest
# Parameters:
# - MANIFEST: the relative path to the manifest file
# - ITEM: the relative path to the item (file, directory, symlink) to be added
# - PATH_IN_FS: the absolute path of the item in the target system when installed
add_to_manifest() { # swupd_function

	add_to_manifest_usage() {
		cat <<-EOM
		Usage:
		    add_to_manifest [-s] [-p] [-e] <manifest> <item> <item_path_in_fs>

		Options:
		    -s    If set (skip), the validation of parameters will be skipped
		    -p    If set (partial), the item will be added to the manifest, but the
		          manifest's tar won't be re-created. If the manifest being updated
		          is the MoM, it won't be re-signed either. This is useful if more
		          updates are to be done in the manifest to avoid extra processing
		    -e    If set (experimental), the bundle will be added to the manifest as
		          experimental. This should only be used when <manifest> is a MoM
		EOM
	}

	local OPTIND
	local opt
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
	local partial=false
	local experimental="."

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		add_to_manifest -h
		return
	fi

	while getopts :spe opt; do
		case "$opt" in
			s)	skip_param_validation=true ;;
			p)	partial=true ;;
			e)	experimental="e" ;;
			*)	add_to_manifest_usage
				return ;;
		esac
	done
	shift $((OPTIND-1))
	local manifest=$1
	local item=$2
	local item_path=$3

	if [ "$skip_param_validation" = false ]; then
		validate_item "$manifest"
		validate_item "$item"
		validate_param "$item_path"
	fi

	item_size=$(stat -c "%s" "$item")
	name=$(basename "$item")
	version=$(basename "$(dirname "$manifest")")
	# add to filecount
	filecount=$(awk '/^filecount/ { print $2}' "$manifest")
	filecount=$((filecount + 1))
	sudo sed -i "s/^filecount:.*/filecount:\\t$filecount/" "$manifest"
	# add to contentsize
	contentsize=$(awk '/^contentsize/ { print $2}' "$manifest")
	contentsize=$((contentsize + item_size))
	# get the item type
	if [ "$(basename "$manifest")" = Manifest.MoM ]; then
		if [[ "$(basename "$item")" == Manifest.*.I.* ]]; then
			item_type=I
		else
			item_type=M
		fi
		# MoM has a contentsize of 0, so don't increase this for MoM
		contentsize=0
		# files, directories and links are stored already hashed, but since
		# manifests are not stored hashed, we need to calculate the hash
		# of the manifest before adding it to the MoM
		name=$(sudo "$SWUPD" hashdump --quiet "$item")
	elif [ -L "$item" ]; then
		item_type=L
		# when adding a link to a bundle, we need to make sure we add
		# its associated file too, unless it is a dangling link
		linked_file=$(readlink "$item")
		if [ -e "$(dirname "$item")"/"$linked_file" ]; then
			if ! sudo cat "$manifest" | grep -q "$linked_file"; then
				file_path="$(dirname "$item_path")"
				if [ "$file_path" = "/" ]; then
					file_path=""
				fi
				add_to_manifest -p "$manifest" "$(dirname "$item")"/"$linked_file" "$file_path"/"$(generate_random_name test-file-)"
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
	sudo sed -i "s/^contentsize:.*/contentsize:\\t$contentsize/" "$manifest"
	# add to manifest content
	write_to_protected_file -a "$manifest" "$item_type$experimental$boot_type.\\t$name\\t$version\\t$item_path\\n"
	# If a manifest tar already exists for that manifest, renew the manifest tar unless specified otherwise
	if [ "$partial" = false ]; then
		retar_manifest "$manifest"
	fi

}

# Adds the specified bundle dependency to an existing bundle manifest
# Parameters:
# - -p: if the p (partial) flag is set the function skips updating the hashes
#       in the MoM, and re-creating the tar, this is useful if more changes are
#       to be done in order to reduce time
# - MANIFEST: the relative path to the manifest file
# - DEPENDENCY: the name of the bundle to be included as a dependency
add_dependency_to_manifest() { # swupd_function

	local partial=false
	local flag=includes
	[ "$1" = "-p" ] && { partial=true ; shift ; }
	[ "$1" = "-o" ] && { flag="also-add"; shift ; }
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
			    add_dependency_to_manifest [-p] [-o] <manifest> <dependency>

			Options:
			    -p    If set (partial), the dependency will be added to the manifest,
			          but the manifest's tar won't be re-created, nor the hash in the
			          MoM will be updated either. This is useful if more updates are
			          to be done in the manifest to avoid extra processing
			    -o    If set (optional), the dependency will be added to the manifest
			          as an optional dependency (also-add flag).

			    Note: if both options -p and -o are to be used, they must be specified in that order or
			      one option will be ignored.
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
				pre_version=$(awk '/^previous/ { print $2 }' "$path"/"$pre_version"/Manifest.MoM)
		done
		sudo cp "$path"/"$pre_version"/"$manifest_name" "$path"/"$version"/"$manifest_name"
		update_manifest -p "$manifest" version "$version"
		update_manifest -p "$manifest" previous "$pre_version"
	fi
	update_manifest -p "$manifest" timestamp "$(date +"%s")"
	sudo sed -i "/^contentsize:.*/a $flag:\\t$dependency" "$manifest"
	# If a manifest tar already exists for that manifest, renew the manifest tar
	# unless specified otherwise
	if [ "$partial" = false ]; then
		retar_manifest "$manifest"
	fi

}

# Removes the specified item from an existing bundle manifest
# Parameters:
# - -p: if the p (partial) flag is set the function skips updating the hashes
#       in the MoM, and re-creating the tar, this is useful if more changes are
#       to be done in order to reduce time
# - MANIFEST: the relative path to the manifest file
# - ITEM: either the hash or filename of the item to be removed
remove_from_manifest() { # swupd_function

	local partial=false
	[ "$1" = "-p" ] && { partial=true ; shift ; }
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

			Options:
			    -p    If set (partial), the item will be removed from the manifest,
			          but the manifest's tar won't be re-created, nor the hash in
			          the MoM will be updated either. This is useful if more updates
			          are to be done in the manifest to avoid extra processing
			EOM
		return
	fi
	validate_item "$manifest"
	validate_param "$item"

	# replace every / with \/ in item (if any)
	item="${item////\\/}"
	# decrease filecount and contentsize
	filecount=$(awk '/^filecount/ { print $2}' "$manifest")
	filecount=$((filecount - 1))
	update_manifest -p "$manifest" filecount "$filecount"
	if [ "$(basename "$manifest")" != Manifest.MoM ]; then
		contentsize=$(awk '/^contentsize/ { print $2}' "$manifest")
		item_hash=$(get_hash_from_manifest "$manifest" "$item")
		item_size=$(stat -c "%s" "$(dirname "$manifest")"/files/"$item_hash")
		contentsize=$((contentsize - item_size))
		update_manifest -p "$manifest" contentsize "$contentsize"
	fi
	# remove the lines that match from the manifest
	sudo sed -i "/\\t$item$/d" "$manifest"
	sudo sed -i "/\\t$item\\t/d" "$manifest"
	# If a manifest tar already exists for that manifest, renew the manifest tar
	# unless specified otherwise
	if [ "$partial" = false ]; then
		retar_manifest "$manifest"
	fi

}

# Updates fields in an existing manifest
# Parameters:
# - -p: if the p (partial) flag is set the function skips updating the hashes
#       in the MoM, this is useful if more changes are to be done in order to
#       reduce time
# - MANIFEST: the relative path to the manifest file
# - KEY: the thing to be updated
# - HASH/NAME: the file name or hash of the record to be updated (if applicable)
# - VALUE: the value to be used for updating the record
update_manifest() { # swupd_function

	local partial=false
	[ "$1" = "-p" ] && { partial=true ; shift ; }
	local manifest=$1
	local key=$2
	local var=$3
	local value=$4
	local manifest_version
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    update_manifest [-p] <manifest> <format | minversion | version | previous | filecount | timestamp | contentsize> <new_value>
			    update_manifest [-p] <manifest> <file-status | file-hash | file-version | file-name> <file_hash or file_name> <new_value>

			Options:
			    -p    if the p flag is set (partial), the function skips updating the MoM's hashes, creating
			          a tar for the MoM and signing it. It also skips creating the tar for the modified manifest,
			          this is useful if more updates are to be done in the manifest to avoid extra processing
			EOM
		return
	fi
	validate_item "$manifest"
	validate_param "$key"
	validate_param "$var"

	var="${var////\\/}"
	value="${value////\\/}"

	case "$key" in
	format)
		sudo sed -i "s/^MANIFEST.*/MANIFEST\\t$var/" "$manifest"
		;;
	version | previous | filecount | timestamp | contentsize)
		sudo sed -i "s/^$key:.*/$key:\\t$var/" "$manifest"
		;;
	minversion)
		# Update minversion field which should only exist in MoM
		sudo sed -i "s/^$key:.*/$key:\\t$var/" "$manifest"

		# Replace manifest version when older than minversion
		manifest_version=$(awk '/^version:/ {print $2}' "$manifest")
		if [ "$manifest_version" -lt "$var" ]; then
			sudo sed -i "/^version:\\t/ s/[0-9]\\+/$var/" "$manifest"
		fi
		# Replace file versions with minversion when less than minversion
		sudo gawk -i inplace -v var="$var" '{if($3 != "" && $3<var) {$3=var} print }' OFS="\\t" "$manifest"
		;;
	file-status)
		validate_param "$value"
		sudo sed -i "/\\t$var$/s/....\\(\\t.*\\t.*\\t.*$\\)/$value\\1/g" "$manifest"
		sudo sed -i "/\\t$var\\t/s/....\\(\\t.*\\t.*\\t.*$\\)/$value\\1/g" "$manifest"
		;;
	file-hash)
		validate_param "$value"
		sudo sed -i "/\\t$var$/s/\\(....\\t\\).*\\(\\t.*\\t\\)/\\1$value\\2/g" "$manifest"
		sudo sed -i "/\\t$var\\t/s/\\(....\\t\\).*\\(\\t.*\\t\\)/\\1$value\\2/g" "$manifest"
		;;
	file-version)
		validate_param "$value"
		sudo sed -i "/\\t$var$/s/\\(....\\t.*\\t\\).*\\(\\t\\)/\\1$value\\2/g" "$manifest"
		sudo sed -i "/\\t$var\\t/s/\\(....\\t.*\\t\\).*\\(\\t\\)/\\1$value\\2/g" "$manifest"
		;;
	file-name)
		validate_param "$value"
		sudo sed -i "/\\t$var$/s/\\(....\\t.*\\t.*\\t\\).*/\\1$value/g" "$manifest"
		sudo sed -i "/\\t$var\\t/s/\\(....\\t.*\\t.*\\t\\).*/\\1$value/g" "$manifest"
		;;
	*)
		terminate "Please select a valid key for updating the manifest"
		;;
	esac
	# update bundle tars and MoM (unless specified otherwise)
	if [ "$partial" = false ]; then
		retar_manifest "$manifest"
	fi

}

# Recalculate the hashes of the elements in the specified MoM and updates it
# if there are changes in hashes.
# Parameters:
# - MANIFEST: the path to the MoM to be updated
update_hashes_in_mom() { # swupd_function

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
		bundles=("$(sudo cat "$manifest" | grep -x "M\\.\\.\\..*" | awk '{ print $4 }')")
		for bundle in ${bundles[*]}; do
			# if the hash of the manifest changed, update it
			bundle_old_hash=$(get_hash_from_manifest "$manifest" "$bundle")
			bundle_new_hash=$(sudo "$SWUPD" hashdump --quiet "$path"/Manifest."$bundle")
			if [ "$bundle_old_hash" != "$bundle_new_hash" ] && [ "$bundle_new_hash" != "$zero_hash" ]; then
				# replace old hash with new hash
				sudo sed -i "/\\t$bundle_old_hash\\t/s/\\(....\\t\\).*\\(\\t.*\\t\\)/\\1$bundle_new_hash\\2/g" "$manifest"
				# replace old version with new version
				sudo sed -i "/\\t$bundle_new_hash\\t/s/\\(....\\t.*\\t\\).*\\(\\t\\)/\\1$(basename "$path")\\2/g" "$manifest"
			fi
		done
		# re-order items on the manifest so they are in the correct order based on version
		sudo sort -t$'\t' -k3 -s -h -o "$manifest" "$manifest"
		# since the MoM has changed, sign it again and update its tar
		retar_manifest "$manifest"
	else
		echo "The provided manifest is not the MoM"
		return 1
	fi
	unset IFS

}

# Update all manifests after a format bump
# Parameters:
# ENVIRONMENT_NAME: the name of the test environment
bump_format() { # swupd_function

	local env_name=$1
	local format
	local version
	local previous_version
	local new_version
	local middle_version
	local new_version_path
	local middle_version_path
	local mom
	local manifest
	local bundles
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    bump_format <environment_name>

			    Note: bump_format can only be used in an environment that has the
			          os-core bundle and release files (create_test_environment -r MY_ENV)
			EOM
		return
	fi
	validate_path "$env_name"

	# find the latest version and MoM
	version=$(get_latest_version "$env_name")
	middle_version="$((version+10))"
	middle_version_path="$env_name"/web-dir/"$middle_version"
	new_version="$((version+20))"
	new_version_path="$env_name"/web-dir/"$new_version"
	format="$(< "$env_name"/web-dir/"$version"/format)"

	# if the environment doesn't have os-core or was not created with release files exit
	manifest="$env_name"/web-dir/"$version"/Manifest.os-core
	if [ ! -e "$manifest" ] || ! grep -qx "F\\.\\.\\..*/usr/lib/os-release$" "$manifest"; then
		terminate "bump_format can only be used in an environment that has the os-core bundle and release files"
	fi

	# create a +10 and a +20 versions for the format bump
	create_version "$env_name" "$middle_version" "$version" "$format"
	create_version -r "$env_name" "$new_version" "$middle_version" "$((format+1))"

	# Update the +10 version
	mom="$middle_version_path"/Manifest.MoM

	# +10 and +20 versions should have matching content
	sudo rm -rf "$middle_version_path"
	sudo cp -r "$new_version_path" "$middle_version_path"

	# Update +10 format and versions
	sudo sh -c "echo $format > $middle_version_path/format"
	update_manifest -p "$mom" format "$format"
	update_manifest -p "$mom" version "$middle_version"
	update_manifest -p "$mom" previous "$version"

	# The +20 version updates the os-release and format files. Change
	# the versions of these files in the +10 to the +10's version.
	update_manifest -p "$mom" file-version os-core "$middle_version"
	update_manifest -p "$middle_version_path"/Manifest.os-core file-version /usr/lib/os-release "$middle_version"
	update_manifest -p "$middle_version_path"/Manifest.os-core file-version /usr/share/defaults/swupd/format "$middle_version"

	mapfile -t bundles < <(awk '/^M\.\.\./ { print $4 }' "$mom")
	IFS=$'\n'
	for bundle in ${bundles[*]}; do
		manifest="$middle_version_path"/Manifest."$bundle"
		update_manifest -p "$manifest" format "$format"
		update_manifest -p "$manifest" version "$middle_version"
		update_manifest "$manifest" previous "$version"
	done
	unset IFS
	update_hashes_in_mom "$mom"

	# update the minversion
	mom="$new_version_path"/Manifest.MoM
	update_manifest -p "$new_version_path"/Manifest.os-core previous "$middle_version"
	set_as_minversion "$new_version_path"

}

# Signs a manifest with a PEM key and generates the signed manifest in the same location
# Parameters:
# - MANIFEST: the path to the manifest to be signed
sign_manifest() { # swupd_function

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
	-signer "$TEST_ROOT_DIR"/Swupd_Root.pem \
	-inkey "$TEST_ROOT_DIR"/private.pem \
	-outform DER -out "$(dirname "$manifest")"/Manifest.MoM.sig
}

# Signs the version file with a PEM key and generates the signature in the same location
# Parameters:
# - FILE: the path for version file to be signed
sign_version() { # swupd_function

	local version_file=$1

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    sign_version <version_file>
			EOM
		return
	fi
	validate_item "$version_file"

	sudo openssl smime -sign -binary -in "$version_file" \
		-signer "$TEST_ROOT_DIR"/Swupd_Root.pem \
		-inkey "$TEST_ROOT_DIR"/private.pem \
		-outform DER -out "$version_file".sig

}

# Retrieves the hash value of a file or directory in a manifest
# Parameters:
# - MANIFEST: the manifest in which it will be looked at
# - ITEM: the dir or file to look for in the manifest
get_hash_from_manifest() { # swupd_function

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
set_current_version() { # swupd_function

	local env_name=$1
	local new_version=$2

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_current_version <environment_name> <new_version>
			EOM
		return
	fi
	validate_path "$env_name"

	sudo sed -i "s/VERSION_ID=.*/VERSION_ID=$new_version/" "$env_name"/testfs/target-dir/usr/lib/os-release

}

# Sets the latest version on the "server" to the desired version
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environmnt to act upon
# - NEW_VERSION: the version for the target to be set to
set_latest_version() { # swupd_function

	local env_name=$1
	local new_version=$2

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_latest_version <environment_name> <new_version>
			EOM
		return
	fi
	validate_path "$env_name"

	write_to_protected_file "$env_name"/web-dir/version/formatstaging/latest "$new_version"
	sign_version "$env_name"/web-dir/version/formatstaging/latest

}

# Creates a new version of the server side content in the specified
# location to serve as a 3rd-party repository
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment
# - VERSION: the version of the server side content
# - FORMAT: the format to use for the version
# - REPO_NAME: the name of the 3rd-party repo, defaults to "test-repo"
create_third_party_repo() { #swupd_function

	local env_name=$1
	local version=$2
	local format=${3:-staging}
	local repo_name=${4:-test-repo}
	local repo_state_dir
	local path

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_third_party_repo <environment_name> <new_version> [format] [repo_name]
			EOM
		return
	fi
	validate_item "$env_name"
	validate_param "$version"

	debug_msg "Creating 3rd-party repo $repo_name"
	path=$(dirname "$(realpath "$env_name")")

	# create the state dir for the 3rd-party repo
	repo_state_dir="$STATEDIR"/3rd_party/"$repo_name"
	debug_msg "Creating a state directory for repo $repo_name at $repo_state_dir"
	sudo mkdir -p "$repo_state_dir"/{staged,download,delta,telemetry,bundles}

	# create the basic content for the 3rd-party repo
	create_version "$env_name" "$version" 0 "$format" "$repo_name"
	debug_msg "3rd-party repo $repo_name created successfully"

	# add the new repo to the repo.ini file
	{
		printf '[%s]\n\n' "$repo_name"
		printf 'URL=%s\n\n' "file://$path/$env_name/3rd_party/$repo_name"
		printf 'VERSION=%s\n\n' "$version"
	} | sudo tee -a "$STATEDIR"/3rd_party/repo.ini > /dev/null

	sudo chmod -R 0700 "$STATEDIR"
	export TPSTATEDIR="$repo_state_dir"
	export TPWEBDIR="$env_name"/3rd_party/"$repo_name"
	debug_msg "3rd-party repo state dir: $TPSTATEDIR"
	debug_msg "3rd-party repo content dir: $TPWEBDIR"

}

# Creates a new version of the server side content
# Parameters:
# - -p: if the p flag is set (partial), the function skips creating the MoM's
#       tar and signing it, this is useful if more changes are to be done in the
#       version in order to avoid extra processing
# - -r: if the r flag is set (release), the version is created with hashed os-release
#       and format files that can be used for creating updates
# - ENVIRONMENT_NAME: the name of the test environment
# - VERSION: the version of the server side content
# - FROM_VERSION: the previous version, if nothing is selected defaults to 0
# - FORMAT: the format to use for the version
# - CONTENT_DIR: the directory where all the content is to be created, defaults to "web-dir"
create_version() { # swupd_function

	local partial=false
	local release_files=false
	# since this function is called with every test environment, some times multiple
	# times, use simple parsing of arguments instead of using getopts to keep light weight
	# with the caveat that arguments need to be provided in a specific order
	[ "$1" = "-p" ] && { partial=true ; shift ; }
	[ "$1" = "-r" ] && { release_files=true ; shift ; }
	local env_name=$1
	local version=$2
	local from_version=${3:-0}
	local format=${4:-staging}
	local content_dir=${5:-web-dir}
	local mom
	local hashed_name
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_version [-p] [-r] <environment_name> <new_version> [from_version] [format] [content_dir]

			Options:
			    -p    if the p flag is set (partial), the function skips creating the MoM's
			          tar and signing it, this is useful if more changes are to be done in the
			          new version in order to avoid extra processing
			    -r    if the r flag is set (release), the version is created with hashed os-release
			          and format files that can be used for creating updates

			Note: if both options -p and -r are to be used, they must be specified in that order or
			      one option will be ignored.
			EOM
		return
	fi
	validate_item "$env_name"
	validate_param "$version"

	# if a content_dir is specified it means we are using a 3rd-party repo
	if [ "$content_dir" != web-dir ]; then
		content_dir=3rd_party/"$content_dir"
	fi

	debug_msg "Creating content for version $version at $content_dir..."

	# if the requested version already exists do nothing
	if [ -d "$env_name"/"$content_dir"/"$version" ]; then
		echo "the requested version $version already exists"
		return
	fi

	sudo mkdir -p "$env_name"/"$content_dir"/"$version"/{files,delta}
	sudo mkdir -p "$env_name"/"$content_dir"/version/format"$format"
	write_to_protected_file "$env_name"/"$content_dir"/version/format"$format"/latest "$version"
	sign_version "$env_name"/"$content_dir"/version/format"$format"/latest
	write_to_protected_file "$env_name"/"$content_dir"/version/latest_version "$version"
	sign_version "$env_name"/"$content_dir"/version/latest_version
	if [ "$format" = staging ]; then
		format=1
	fi
	write_to_protected_file "$env_name"/"$content_dir"/"$version"/format "$format"
	# create a new os-release file per version
	{
		printf 'NAME="Swupd Test Distro"\n'
		printf 'VERSION=1\n'
		printf 'ID=clear-linux-os\n'
		printf 'VERSION_ID=%s\n' "$version"
		printf 'PRETTY_NAME="Swupd Test Distro"\n'
		printf 'ANSI_COLOR="1;35"\n'
		printf 'HOME_URL="https://clearlinux.org"\n'
		printf 'SUPPORT_URL="https://clearlinux.org"\n'
		printf 'BUG_REPORT_URL="https://bugs.clearlinux.org/jira"\n'
	} | sudo tee "$env_name"/"$content_dir"/"$version"/os-release > /dev/null
	# copy hashed versions of os-release and format to the files directory
	if [ "$release_files" = true ]; then
		debug_msg "Creating release files"
		hashed_name=$(sudo "$SWUPD" hashdump --quiet "$env_name"/"$content_dir"/"$version"/os-release)
		sudo cp "$env_name"/"$content_dir"/"$version"/os-release "$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		create_tar "$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		OS_RELEASE="$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		export OS_RELEASE
		hashed_name=$(sudo "$SWUPD" hashdump --quiet "$env_name"/"$content_dir"/"$version"/format)
		sudo cp "$env_name"/"$content_dir"/"$version"/format "$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		create_tar "$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		FORMAT="$env_name"/"$content_dir"/"$version"/files/"$hashed_name"
		export FORMAT
	fi
	# if the previous version is 0 then create a new MoM, otherwise copy the MoM
	# from the previous version
	if [ "$from_version" = 0 ]; then

		debug_msg "This is the first version in the content directory, creating a new MoM..."
		mom=$(create_manifest "$env_name"/"$content_dir"/"$version" MoM)
		if [ "$partial" = false ]; then
			create_tar "$mom"
			sign_manifest "$mom"
		fi
		debug_msg "MoM created successfully"

	else

		# copy the packs from the previous version
		debug_msg "Creating a new MoM for version $version based on the MoM from version $from_version"
		sudo cp "$env_name"/"$content_dir"/"$from_version"/pack-*.tar "$env_name"/"$content_dir"/"$version"

		sudo cp "$env_name"/"$content_dir"/"$from_version"/Manifest.MoM "$env_name"/"$content_dir"/"$version"
		mom="$env_name"/"$content_dir"/"$version"/Manifest.MoM
		# update MoM info and create the tars
		update_manifest -p "$mom" format "$format"
		update_manifest -p "$mom" version "$version"
		update_manifest -p "$mom" previous "$from_version"
		update_manifest -p "$mom" timestamp "$(date +"%s")"
		if [ "$partial" = false ]; then
			create_tar "$mom"
			sign_manifest "$mom"
		fi
		# when creating a new version we need to make an update to os-release and format
		# files in the os-core bundle (unless otherwise specified or if bundle does not exist)
		if [ "$release_files" = true ]; then
			oldversion="$version"
			while [ "$oldversion" -gt 0 ] && [ ! -e "$env_name"/"$content_dir"/"$oldversion"/Manifest.os-core ]; do
				oldversion=$(awk '/^previous/ { print $2 }' "$env_name"/"$content_dir"/"$oldversion"/Manifest.MoM)
			done
			# if no os-core manifest was found, nothing else needs to be done
			if [ -e "$env_name"/"$content_dir"/"$oldversion"/Manifest.os-core ]; then
				update_bundle -p "$env_name" os-core --header-only
				if [ -n "$(get_hash_from_manifest "$env_name"/"$content_dir"/"$oldversion"/Manifest.os-core /usr/lib/os-release)" ]; then
					remove_from_manifest -p "$env_name"/"$content_dir"/"$version"/Manifest.os-core /usr/lib/os-release
				fi
				update_bundle -p "$env_name" os-core --add /usr/lib/os-release:"$OS_RELEASE"
				if [ -n "$(get_hash_from_manifest "$env_name"/"$content_dir"/"$oldversion"/Manifest.os-core /usr/share/defaults/swupd/format)" ]; then
					remove_from_manifest -p "$env_name"/"$content_dir"/"$version"/Manifest.os-core /usr/share/defaults/swupd/format
				fi
				# update without -p flag to refresh tar and MoM
				update_bundle "$env_name" os-core --add /usr/share/defaults/swupd/format:"$FORMAT"
			fi
		fi
		debug_msg "MoM created successfully"

	fi

}

# Creates a test environment with the basic directory structure needed to
# validate the swupd client
# Parameters:
# - -e: if this option is set the test environment is created empty (withouth bundle os-core)
# - -r: if this option is set the test environment is created with a more complete version of
#       the os-core bundle that includes release files, it is useful for some tests like update tests
# - ENVIRONMENT_NAME: the name of the test environment, this should be typically the test name
# - VERSION: the version to use for the test environment, if not specified the default is 10
# - FORMAT: the format number to use initially in the environment
create_test_environment() { # swupd_function

	cte_usage() {
		cat <<-EOM
			Usage:
			    create_test_environment [-e|-r] [-s <size in MB>] <environment_name> [initial_version] [format]

			Options:
			    -e    If set, the test environment is created empty, otherwise it will have
			          bundle os-core in the web-dir and installed by default.
			    -r    If set, the test environment is created with a more complete version of
			          the os-core bundle, a version that includes the os-release and format
			          files, so it is more useful for some tests, like update tests.
			    -s    If used, specifies the maximum size the test environment has in MB, this
			          can be useful when testing scenarios bound to the disk size.

			Note: options -e and -r are mutually exclusive, so you can only use one at a time.
		EOM
	}

	local OPTIND
	local opt
	local empty=false
	local release_files=false
	local size=0
	local path
	local targetdir
	local statedir

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		create_test_environment -h
		return
	fi

	while getopts :ers: opt; do
		case "$opt" in
			e)	empty=true
				release_files=false ;;
			r)	release_files=true
				empty=false ;;
			s)	size="$OPTARG" ;;
			*)	cte_usage
				return ;;
		esac
	done
	shift $((OPTIND-1))

	local env_name=$1
	local version=${2:-10}
	local format=${3:-staging}
	validate_param "$env_name"
	validate_number "$size"

	debug_msg "Creating test environment: $env_name"

	# clean test environment when test interrupted
	trap 'destroy_test_environment $TEST_NAME' INT

	# create all the files and directories needed
	# web-dir files & dirs
	sudo mkdir -p "$env_name"
	if [ "$release_files" = true ]; then
		create_version -p -r "$env_name" "$version" "0" "$format"
	else
		create_version -p "$env_name" "$version" "0" "$format"
	fi

	# if a size was selected, create the FS,
	# if not just use a normal directory
	if [ "$size" -gt 0 ]; then
		print "Creating a test file system..."
		print "Created: $(create_test_fs "$env_name" "$size") ($size MB)"
		sudo touch "$env_name"/.testfs
	else
		debug_msg "No size restrictions were selected for the environment, using a common directory as FS"
		sudo mkdir -p "$env_name"/testfs
	fi
	targetdir="$env_name"/testfs/target-dir
	statedir="$env_name"/testfs/state

	# target-dir files & dirs
	debug_msg "Creating target-dir files and diretories"
	path=$(dirname "$(realpath "$env_name")")
	sudo mkdir -p "$targetdir"/usr/lib
	sudo cp "$env_name"/web-dir/"$version"/os-release "$targetdir"/usr/lib/os-release
	sudo mkdir -p "$targetdir"/usr/share/clear/bundles
	sudo mkdir -p "$targetdir"/usr/share/defaults/swupd
	sudo cp "$env_name"/web-dir/"$version"/format "$targetdir"/usr/share/defaults/swupd/format
	write_to_protected_file "$targetdir"/usr/share/defaults/swupd/versionurl "file://$path/$env_name/web-dir"
	write_to_protected_file "$targetdir"/usr/share/defaults/swupd/contenturl "file://$path/$env_name/web-dir"
	sudo mkdir -p "$targetdir"/etc/swupd

	# state files & dirs
	debug_msg "Creating a state dir"
	sudo mkdir -p "$statedir"/{staged,download,delta,telemetry,bundles,3rd_party}
	sudo chmod -R 0700 "$statedir"

	# export environment variables that are dependent of the test env
	set_env_variables "$env_name"

	# every environment needs to have at least the os-core bundle so this should be
	# added by default to every test environment unless specified otherwise
	if [ "$empty" = false ]; then
		debug_msg "Adding bundle os-core to the test environment"
		if [ "$release_files" = true ]; then
			debug_msg "Including the release files in bundle os-core"
			create_bundle -L -n os-core -v "$version" -d /usr/share/clear/bundles -f /core,/usr/lib/os-release:"$OS_RELEASE",/usr/share/defaults/swupd/format:"$FORMAT" "$env_name"
		else
			create_bundle -L -n os-core -v "$version" -f /core "$env_name"
		fi
	else
		debug_msg "The -e flag was specified, the os-core bundle was not included in the environment"
		create_tar "$env_name"/web-dir/"$version"/Manifest.MoM
		sign_manifest "$env_name"/web-dir/"$version"/Manifest.MoM
	fi

	debug_msg "Test environment \"$env_name\" created successfully"

}

# Destroys a test environment
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment to be deleted
destroy_test_environment() { # swupd_function

	local force=false

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    destroy_test_environment [--force] <environment_name>

			Options:
			    --force    If set, the environment will be destroyed even if KEEP_ENV=true

			Note: if you want your test environment to be preserved for debugging
			      purposes, set KEEP_ENV=true before running your test.
			EOM
		return
	fi
	[ "$1" = "--force" ] && { force=true ; shift ; }
	local env_name=$1
	validate_param "$env_name"
	debug_msg "Destroying environment $env_name"
	if [ -z "$KEEP_ENV" ]; then
		KEEP_ENV=false
	fi
	debug_msg "The KEEP_ENV environment variable is set to $KEEP_ENV"
	if [ "$force" = true ]; then
		debug_msg "The --force option was used"
	fi

	destroy_web_server
	destroy_trusted_cacert

	# if the test environment doesn't exist warn the user but don't terminate script
	# execution, this function could be called from a test teardown and terminating
	# at that point will cause incorrect test reporting
	if [ ! -d "$env_name" ]; then
		echo "Warning: test environment \"$env_name\" doesn't exist, nothing to destroy."
		return 1
	fi

	if [ "$KEEP_ENV" = true ] && [ "$force" = false ]; then
		local msg="Warning: KEEP_ENV is set to true, the test environment will be preserved"
		print "$msg\\n" 2>/dev/null || echo "$msg"
		return
	fi

	# since the action to be performed is very destructive, at least
	# make sure the directory does look like a test environment
	for var in "testfs" "web-dir"; do
		if [ ! -d "$env_name"/"$var" ]; then
			echo "The name provided \"$env_name\" doesn't seem to be a valid test environment"
			return 1
		fi
	done

	# if a test fs was created, destroy it
	if [ -e "$env_name"/.testfs ]; then
		destroy_test_fs "$env_name"
	fi

	sudo rm -rf "$env_name"
	if [ ! -e "$env_name" ]; then
		debug_msg "Environment $env_name deleted successfully"
	else
		debug_msg "The environment $env_name failed to be deleted"
	fi

}

# Creates a test fiile system of a given size
create_test_fs() { # swupd_function

	local env_name=$1
	local size=$2
	local fsfile
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_test_fs <environment_name> <size_in_MB>
			EOM
		return
	fi
	validate_path "$env_name"
	validate_param "$size"
	fsfile=/tmp/"$env_name"

	# create a file of the appropriate size
	dd if=/dev/zero of="$fsfile" bs=1M count="$size" >& /dev/null
	# create loop device and format it
	sudo losetup -f "$fsfile"
	mkfs.ext4 "$fsfile" >& /dev/null
	# mount the fs
	sudo mkdir "$env_name"/testfs
	sudo mount "$fsfile" "$env_name"/testfs -o sync

	# return the fs
	echo "$env_name"/testfs

}

destroy_test_fs() { # swupd_function

	local env_name=$1
	local fsfile
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    destroy_test_fs <environment_name>
			EOM
		return
	fi
	validate_path "$env_name"
	fsfile=/tmp/"$env_name"

	sudo umount "$env_name"/testfs
	sudo losetup -d "$(losetup -j "$fsfile" -nO NAME)"
	rm "$fsfile"

}

create_config_file() { # swupd_function

	if [ ! -d "$TEST_NAME" ]; then
		terminate "The TEST_NAME needs to be specified to create a config file"
	fi

	# these values have to be exported in this function because the
	# TEST_NAME changes during the setup
	export SWUPD_CONFIG_DIR="$TEST_NAME"/testconfig
	export SWUPD_CONFIG_FILE="$SWUPD_CONFIG_DIR"/config
	debug_msg "Creating config file $SWUPD_CONFIG_FILE..."

	if [ "$("$SWUPD" --version | grep "config file path" | awk '{print $4}')" != "./testconfig" ]; then
		skip "Tests that use a config file require swupd to be built with '--with-config-file-path=./testconfig'"
	fi

	if [ -e "$SWUPD_CONFIG_FILE" ]; then
		debug_msg "A config file already existed in that location, deleting it..."
		destroy_config_file
	fi

	sudo mkdir -p "$SWUPD_CONFIG_DIR"
	sudo touch "$SWUPD_CONFIG_FILE"
	debug_msg "Config file created successfully"

}

destroy_config_file() { # swupd_function

	if [ ! -d "$SWUPD_CONFIG_DIR" ]; then
		print "There was no directory $SWUPD_CONFIG_DIR, nothing was deleted"
		return
	fi

	debug_msg "Deleting config file directory $SWUPD_CONFIG_DIR"
	sudo rm -rf "$SWUPD_CONFIG_DIR"
	# we need to make sure the config file is deleted before
	# another test is run so it does not interfere with it since
	# the config file is common and shared by all commands
	wait_for_deletion "$SWUPD_CONFIG_DIR"
	debug_msg "Config file directory deleted successfully"

}

add_option_to_config_file() { # swupd_function

	local option=$1
	local value=$2
	local section=$3

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    add_option_to_config_file <option> <value> <section_name>
			EOM
		return
	fi

	validate_param "$option"
	validate_param "$value"

	if [ -n "$section" ]; then
		write_to_protected_file -a "$SWUPD_CONFIG_FILE" "[$section]\n"
	fi
	write_to_protected_file -a "$SWUPD_CONFIG_FILE" "$option=$value\n"

}


# creates a mirror of whatever is in web-dir
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment where the mirror will be created
create_mirror() { # swupd_function

	local env_name=$1
	local path

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_mirror <environment_name>
			EOM
		return
	fi
	validate_param "$env_name"
	path=$(dirname "$(realpath "$env_name")")

	# create the mirror
	sudo mkdir "$env_name"/mirror
	sudo cp -r "$env_name"/web-dir "$env_name"/mirror
	export MIRROR="$env_name"/mirror/web-dir

	# set the mirror in the target-dir
	write_to_protected_file "$env_name"/testfs/target-dir/etc/swupd/mirror_versionurl "file://$path/$MIRROR"
	write_to_protected_file "$env_name"/testfs/target-dir/etc/swupd/mirror_contenturl "file://$path/$MIRROR"

}

# sets the url of where the version will be retrieved
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment
# - VERSION_URL: the url from where the version will be retrieved. Examples:
#        file://some_path/test_environment/web-dir
#        http://some_server:port/some_path/test_environment/web-dir
set_version_url() { # swupd_function

	local env_name=$1
	local version_url=$2
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_version_url <environment_name> <version_url>

			Example:
			    set_version_url "\$TEST_ENVIRONMENT" http://server:port/path/web-dir
			EOM
		return
	fi
	validate_path "$env_name"
	validate_param "$version_url"

	write_to_protected_file "$env_name"/testfs/target-dir/usr/share/defaults/swupd/versionurl "$version_url"

}

# sets the url of where the content will be retrieved
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment
# - VERSION_URL: the url from where the content will be retrieved. Examples:
#        file://some_path/test_environment/web-dir
#        http://some_server:port/some_path/test_environment/web-dir
set_content_url() { # swupd_function

	local env_name=$1
	local content_url=$2
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_content_url <environment_name> <content_url>

			Example:
			    set_content_url "\$TEST_ENVIRONMENT" http://server:port/path/web-dir
			EOM
		return
	fi
	validate_path "$env_name"
	validate_param "$content_url"

	write_to_protected_file "$env_name"/testfs/target-dir/usr/share/defaults/swupd/contenturl "$content_url"

}

# sets the url of the default upstream server
# Parameters:
# - ENVIRONMENT_NAME: the name of the test environment
# - URL: the url of the default upstream server
#   Examples:
#        file://some_path/test_environment/web-dir
#        http://some_server:port/some_path/test_environment/web-dir
set_upstream_server() { # swupd_function

	local env_name=$1
	local url=$2
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    set_upstream_server <environment_name> <server_url>

			Example:
			    set_upstream_server "\$TEST_ENVIRONMENT" http://server:port/path/web-dir
			EOM
		return
	fi
	validate_path "$env_name"
	validate_param "$url"

	set_version_url "$env_name" "$url"
	set_content_url "$env_name" "$url"

}

# creates a web server to host fake swupd content with or without certifiates
start_web_server() { # swupd_function

	local OPTIND
	local opt
	local port
	local server_args
	local status

	web_server_usage() {
		cat <<-EOF
		Usage:
		    start_web_server [-c] <client pub key> [-d] <file name> [-k] <server priv key> [-p] <server pub key>
		                     [-s] [-H] [-t] <delay value> [-l] <chunk length> [-r] [-P] <port> [-D] <dir to server>
		                     [-n] <number of normal requests> [-f] <HTTP code>

		Options:
		    -c    Path to public key to be used for client certificate authentication
		    -p    Path to server public key which enables SSL authentication
		    -k    Path to server private key which must correspond to the provided server public key
		    -P    If specified, the server will attempt to use the selected port
		    -r    Reachable from other machines in the network
		    -D    The relative path to the directory to serve. If not specified, the directory to
		          serve is the current working directory
		    -s    Use a slow server
		    -n    The server will respond normally for the first N number of requests, and will start
		          responding slowly / or hang after that
		    -t    Used to fine tune the slow server function. It sets the delay value (in miliseconds)
		          used to sleep between reads
		    -l    Used to fine tune the slow server function. It sets the length of the chunk of data
		          read in each cycle by the server
		    -H    Hangs the server
		    -d    File name that will be partially downloaded on the first attempt and successfully
		          downloaded on the second attempt
		    -f    Forces the web server to respond with a specific code to any request

		Notes:
		    - When the server is using SSL authentication, a pair of corresponding public and private keys must be passed
		      as arguments.

		Example of usage:

		    The following command will create a web server that uses SSL authentication and inserts a delay for content
		    updates.

		    start_web_server -k /private_key.pem -p /public_key.pem -s

		EOF
	}

	while getopts :c:d:k:p:st:rP:D:l:n:Hf: opt; do
		case "$opt" in
			c)	server_args="$server_args --client-cert $OPTARG" ;;
			d)	server_args="$server_args --partial-download-file $OPTARG" ;;
			k)	server_args="$server_args --server-key $OPTARG" ;;
			p)	server_args="$server_args --server-cert $OPTARG" ;;
			s)	server_args="$server_args --slow-server" ;;
			t)	server_args="$server_args --time-delay $OPTARG" ;;
			r)	server_args="$server_args --reachable" ;;
			P)	server_args="$server_args --port $OPTARG" ;;
			D)	server_args="$server_args --directory $OPTARG" ;;
			l)	server_args="$server_args --chunk-length $OPTARG" ;;
			n)	server_args="$server_args --after-requests $OPTARG" ;;
			H)	server_args="$server_args --hang-server" ;;
			f)	server_args="$server_args --force-response $OPTARG" ;;
			*)	web_server_usage
				return ;;
		esac
	done

	# start web server and write port/pid numbers to their respective files
	if [ -n "$PORT_FILE" ] && [ -n "$SERVER_PID_FILE" ]; then
		sudo sh -c "python3 $FUNC_DIR/server.py $server_args --port-file $PORT_FILE --pid-file $SERVER_PID_FILE &"
	else
		sudo sh -c "python3 $FUNC_DIR/server.py $server_args &"
	fi

	# make sure localhost is present in no_proxy settings
	if [ -n "$no_proxy" ] && grep -v 'localhost' <<< "$no_proxy"; then
		no_proxy="${no_proxy},localhost"
	elif [ -z "$no_proxy" ]; then
		no_proxy="localhost"
	fi
	export no_proxy

	# wait for server to be available
	for i in $(seq 1 100); do
		if [ -f "$PORT_FILE" ]; then
			port=$(get_web_server_port "$TEST_NAME")
			status=0

			# use https when the server is using certificates
			if [[ "$server_args" = *"--server-cert"* ]]; then
				# to avoid hanging the server while testing for the connection to be ready
				# when the -H (hang server) option is set, we need to use the special url
				# "/test-connection"
				curl https://localhost:"$port"/test-connection > /dev/null --silent || status=$?
			else
				curl http://localhost:"$port"/test-connection > /dev/null --silent || status=$?
			fi

			# the web server is ready for connections when 0 or 60 is returned. When using
			# https, exit status 60 will be returned because certificates are not included
			# in the curl test command
			if [ "$status" -eq 0 ] || [ "$status" -eq 60 ]; then
				break
			fi
		fi

		if [ "$i" -eq 100 ]; then
			echo "Timeout: web server not ready"
			return 1
		fi

		sleep 1
	done

	print "Web server port: $port"
	print "Web server PID: $(cat "$SERVER_PID_FILE")"

}

# gets the port used by the web server if found
get_web_server_port() { # swupd_function

	local env_name=$1

	if [ -z "$PORT_FILE" ] && [ -z "$env_name" ]; then
		cat <<-EOM
			Usage:
		        get_web_server_port [ENV_NAME]

			Note: the ENV_NAME does not need to be specified if the PORT_FILE env
			      variable is set.

			EOM
		terminate "No port file was found. Please specify a test environment"
	fi

	if [ -z "$PORT_FILE" ] && [ -n "$env_name" ]; then
		PORT_FILE="$env_name/port_file.txt"
	fi

	if [ ! -e "$PORT_FILE" ]; then
		terminate "The specified port file was not found"
	fi

	cat "$PORT_FILE"

}

# kills the test web server and removes the files it creates
destroy_web_server() { # swupd_function

	if [ -f "$SERVER_PID_FILE" ]; then
		sudo kill "$(<"$SERVER_PID_FILE")"
	fi

}

# Creates a bundle in the test environment. The bundle can contain files, directories or symlinks.
create_bundle() { # swupd_function

	cb_usage() {
		cat <<-EOM
		Usage:
		    create_bundle [-L] [-t] [-e] [-n] <bundle_name> [-v] <version> [-u] <3rd-party repo> [-d] <list of dirs> [-f] <list of files> [-l] <list of links> ENV_NAME

		Options:
		    -L    When the flag is selected the bundle will be 'installed' in the target-dir, otherwise it will only be created in web-dir
		    -t    The bundle will be 'tracked' in the state-dir regardless of if its content is installed on the system
		    -e    The bundle will be marked as experimental
		    -n    The name of the bundle to be created, if not specified a name will be autogenerated
		    -v    The version for the bundle, if non selected version 10 will be used
		    -u    If the bundle is to be created inside a 3rd-party repo, the repo's name should be specified here
		    -d    Comma-separated list of directories to be included in the bundle
		    -f    Comma-separated list of files to be created and included in the bundle
		    -l    Comma-separated list of symlinks to files to be created and included in the bundle
		    -c    Comma-separated list of symlinks to directories to be created and included in the bundle
		    -b    Comma-separated list of dangling (broken) symlinks to be created and included in the bundle

		Notes:
		    - if no option is selected, a minimal bundle will be created with only one directory
		    - for every symlink created a related file will be created and added to the bundle as well (except for dangling links)
		    - if the '-f' or '-l' options are used, and the directories where the files live don't exist,
		      they will be automatically created and added to the bundle for each file
		    - if instead of creating a new file you want to reuse an existing one, you can do this by adding ':' followed by the
		      path to the file, for example '-f /usr/bin/test-1:my_environment/web-dir/10/files/\$file_hash'

		Example of usage:

		    The following command will create a bundle named 'test-bundle', which will include three directories,
		    four files, and one symlink (they will be added to the bundle's manifest), all these resources will also
		    be tarred. The manifest will be added to the MoM.

		    create_bundle -n test-bundle -f /usr/bin/test-1,/usr/bin/test-2,/etc/systemd/test-3 -l /etc/test-link my_test_env

		EOM
	}

	add_dirs() {

		if [[ "$val" != "/"* ]]; then
			val=/"$val"
		fi
		# if the directories the file is don't exist, add them to the bundle
		# do not add all the directories of the tracking file /usr/share/clear/bundles,
		# this file is added in every bundle by default, it would add too much overhead
		# for most tests (except for 3rd-party)
		fdir=$(dirname "${val%:*}")
		if ! sudo cat "$manifest" | grep -qx "[DL]\\.\\.\\..*$fdir" && [ "$fdir" != "/" ]; then
			if [ "$third_party" = false ] && [ "$fdir" = "/usr/share/clear/bundles" ]; then
				return
			fi
			bundle_dir=$(create_dir "$files_path")
			add_to_manifest -p "$manifest" "$bundle_dir" "$fdir"
			# add each one of the directories of the path if they are not in the manifest already
			while [ "$(dirname "$fdir")" != "/" ]; do
				fdir=$(dirname "$fdir")
				if ! sudo cat "$manifest" | grep -qx "[DL]\\.\\.\\..*$fdir"; then
					add_to_manifest -p "$manifest" "$bundle_dir" "$fdir"
				fi
			done
		fi

	}

	local OPTIND
	local opt
	local dir_list
	local file_list
	local link_list
	local dangling_link_list
	local dlink_list
	local version
	local bundle_name
	local env_name
	local files_path
	local version_path
	local manifest
	local local_bundle=false
	local experimental=false
	local track_bundle=false
	local pfile
	local pfile_name
	local pfile_path
	local dir_name
	local dir_path
	local content_dir
	local third_party=false

	# If no parameters are received show help
	if [ $# -eq 0 ]; then
		create_bundle -h
		return
	fi
	set -f  # turn off globbing
	while getopts :v:d:f:l:b:c:n:u:tLe opt; do
		case "$opt" in
			d)	IFS=, read -r -a dir_list <<< "$OPTARG"  ;;
			f)	IFS=, read -r -a file_list <<< "$OPTARG" ;;
			l)	IFS=, read -r -a link_list <<< "$OPTARG" ;;
			b)	IFS=, read -r -a dangling_link_list <<< "$OPTARG" ;;
			c)	IFS=, read -r -a dlink_list <<< "$OPTARG" ;;
			n)	bundle_name="$OPTARG" ;;
			v)	version="$OPTARG" ;;
			u)	content_dir="$OPTARG"; third_party=true ;;
			t)	track_bundle=true ;;
			L)	local_bundle=true ;;
			e)	experimental=true ;;
			*)	cb_usage
				return ;;
		esac
	done
	set +f  # turn globbing back on
	env_name=${*:$OPTIND:1}

	# if a 3rd-party repo was specified, the bundle should be created in there
	if [ "$third_party" = true ]; then
		target_path="$env_name"/testfs/target-dir/opt/3rd_party/"$content_dir"
		content_dir=3rd_party/"$content_dir"
	else
		target_path="$env_name"/testfs/target-dir
		content_dir=web-dir
	fi

	# set default values
	bundle_name=${bundle_name:-$(generate_random_name test-bundle-)}
	debug_msg "Creating bundle: $bundle_name"
	# if no version was provided create the bundle in the earliest version by defaulti
	# shellcheck disable=SC2010
	# SC2010: Don't use ls | grep. Use a glob.
	# Exception: ls has sorting options that are tricky
	# to get right with other commands.
	version=${version:-$(ls "$env_name"/"$content_dir" | grep -E '^[0-9]+$' | sort -rn | head -n1)}
	# all bundles should include their own tracking file, so append it to the
	# list of files to be created in the bundle
	file_list+=(/usr/share/clear/bundles/"$bundle_name")

	# get useful paths
	validate_path "$env_name"
	version_path="$env_name"/"$content_dir"/"$version"
	files_path="$version_path"/files
	state_path="$env_name"/testfs/state

	# 1) create the initial manifest
	debug_msg "Creating the initial manifest for the bundle"
	manifest=$(create_manifest "$version_path" "$bundle_name")
	if [ "$DEBUG" == true ]; then
		echo "Manifest -> $manifest"
	fi
	# update format in the manifest
	update_manifest -p "$manifest" format "$(cat "$version_path"/format)"

	# 2) Create one directory for the bundle and add it the requested
	# times to the manifest.
	# Every bundle has to have at least one directory,
	# hashes in directories vary depending on owner and permissions,
	# so one directory hash can be reused many times
	debug_msg "Creating the mandatory directory included in every bundle"
	bundle_dir=$(create_dir "$files_path")
	debug_msg "Directory -> $bundle_dir"
	# Create a zero pack for the bundle and add the directory to it
	debug_msg "Creating a zero pack for the bundle and adding the directory to it"
	sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$bundle_dir")"
	for val in "${dir_list[@]}"; do
		add_dirs
		if [ "$val" != "/" ]; then
			add_to_manifest -p "$manifest" "$bundle_dir" "$val"
			if [ "$local_bundle" = true ]; then
				sudo mkdir -p "$target_path$val"
			fi
		fi
	done

	# 3) Create the requested link(s) to directories in the bundle
	for val in "${dlink_list[@]}"; do
		if [[ "$val" != "/"* ]]; then
			val=/"$val"
		fi
		# if the directory the link is doesn't exist,
		# add it to the bundle (except if the directory is "/")
		fdir=$(dirname "$val")
		if [ "$fdir" != "/" ]; then
			if ! sudo cat "$manifest" | grep -qx "[DL]\\.\\.\\..*$fdir"; then
				bundle_dir=$(create_dir "$files_path")
				add_to_manifest -p "$manifest" "$bundle_dir" "$fdir"
			fi
		else
			fdir=""
		fi
		# first create the directory that will be pointed by the symlink
		bundle_dir=$(create_dir "$files_path")
		dir_name=$(generate_random_name dir_)
		dir_path="$fdir"/"$dir_name"
		# temporarily rename the dir to the name it will have in the file system
		# so we can create the symlink with the correct name
		sudo mv "$bundle_dir" "$(dirname "$bundle_dir")"/"$dir_name"
		bundle_link=$(create_link "$files_path" "$(dirname "$bundle_dir")"/"$dir_name")
		sudo mv "$(dirname "$bundle_dir")"/"$dir_name" "$bundle_dir"
		# add the pointed file to the manifest (no need to add it to the pack)
		add_to_manifest "$manifest" "$bundle_dir" "$dir_path"
		# add the symlink to zero pack and manifest
		sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$bundle_link")"
		add_to_manifest -s "$manifest" "$bundle_link" "$val"
		if [ "$DEBUG" == true ]; then
			echo "link -> $bundle_link"
			echo "directory pointed to -> $bundle_dir"
		fi
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled copy the link to target-dir but also
			# copy the dir it points to
			sudo cp -r "$bundle_dir" "$target_path$dir_path"
			sudo ln -rs "$target_path$dir_path" "$target_path$val"
		fi
	done

	# 4) Create the requested file(s)
	for val in "${file_list[@]}"; do
		add_dirs
		# if the user wants to use an existing file, use it, else create a new one
		if [[ "$val" = *":"* ]]; then
			bundle_file="${val#*:}"
			val="${val%:*}"
			validate_item "$bundle_file"
		else
			bundle_file=$(create_file "$files_path")
		fi
		debug_msg "file -> $bundle_file"
		add_to_manifest -p "$manifest" "$bundle_file" "$val"
		# Add the file to the zero pack of the bundle
		sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$bundle_file")"
		# if the local_bundle flag is set, copy the files to the target-dir as if the
		# bundle had been locally installed
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			sudo cp "$bundle_file" "$target_path$val"
		fi
	done

	# 5) Create the requested link(s) to files in the bundle
	for val in "${link_list[@]}"; do
		if [[ "$val" != "/"* ]]; then
			val=/"$val"
		fi
		# if the directory the link is doesn't exist,
		# add it to the bundle (except if the directory is "/")
		fdir=$(dirname "$val")
		if [ "$fdir" != "/" ]; then
			if ! sudo cat "$manifest" | grep -qx "[DL]\\.\\.\\..*$fdir"; then
				bundle_dir=$(create_dir "$files_path")
				add_to_manifest -p "$manifest" "$bundle_dir" "$fdir"
			fi
		else
			fdir=""
		fi
		# first create the file that will be pointed by the symlink
		pfile=$(create_file "$files_path")
		pfile_name=$(generate_random_name file_)
		pfile_path="$fdir"/"$pfile_name"
		# temporarily rename the file to the name it will have in the file system
		# so we can create the symlink with the correct name
		sudo mv "$pfile" "$(dirname "$pfile")"/"$pfile_name"
		bundle_link=$(create_link "$files_path" "$(dirname "$pfile")"/"$pfile_name")
		sudo mv "$(dirname "$pfile")"/"$pfile_name" "$pfile"
		# add the pointed file to the zero pack and manifest
		sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$pfile")"
		add_to_manifest "$manifest" "$pfile" "$pfile_path"
		# add the symlink to zero pack and manifest
		sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$bundle_link")"
		add_to_manifest -s "$manifest" "$bundle_link" "$val"
		debug_msg "link -> $bundle_link"
		debug_msg "file pointed to -> $pfile"
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled copy the link to target-dir but also
			# copy the file it points to
			sudo cp "$pfile" "$target_path$pfile_path"
			sudo ln -rs "$target_path$pfile_path" "$target_path$val"
		fi
	done

	# 6) Create the requested dangling link(s) in the bundle
	for val in "${dangling_link_list[@]}"; do
		if [[ "$val" != "/"* ]]; then
			val=/"$val"
		fi
		# if the directory the link is doesn't exist,
		# add it to the bundle (except if the directory is "/")
		fdir=$(dirname "$val")
		if [ "$fdir" != "/" ]; then
			if ! sudo cat "$manifest" | grep -qx "[DL]\\.\\.\\..*$fdir"; then
				bundle_dir=$(create_dir "$files_path")
				add_to_manifest -p "$manifest" "$bundle_dir" "$fdir"
			fi
		else
			fdir=""
		fi
		# Create a link passing a file that does not exits
		bundle_link=$(create_link "$files_path" "$files_path"/"$(generate_random_name does_not_exist-)")
		sudo tar -C "$files_path" -rf "$version_path"/pack-"$bundle_name"-from-0.tar --transform "s,^,staged/," "$(basename "$bundle_link")"
		add_to_manifest -s "$manifest" "$bundle_link" "$val"
		# Add the file pointed by the link to the zero pack of the bundle
		debug_msg "dangling link -> $bundle_link"
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled since we cannot copy a bad link create a new one
			# in the appropriate location in target-dir with the corrent name
			sudo ln -s "$(generate_random_name /does_not_exist-)" "$target_path$val"
		fi
	done

	# 7) Add the bundle to the MoM (do not use -p option so the MoM's tar is created and signed)
	debug_msg "Adding bundle to the MoM"
	if [ "$experimental" = true ]; then
		add_to_manifest -e "$version_path"/Manifest.MoM "$manifest" "$bundle_name"
	else
		add_to_manifest "$version_path"/Manifest.MoM "$manifest" "$bundle_name"
	fi

	# 8) Create/renew manifest tars
	debug_msg "Creating the manifest tar"
	sudo rm -f "$manifest".tar
	create_tar "$manifest"

	# 9) Create the subscription to the bundle if the local_bundle flag is enabled
	# all installed bundles in the system should have a file in this directory
	if [ "$local_bundle" = true ]; then
		debug_msg "Creating tracking file for bundle so it show as installed"
		sudo touch "$target_path"/usr/share/clear/bundles/"$bundle_name"
	fi

	# 10) Create the tracking file for the bundle if the track_bundle flag is set
	# all bundles specifically installed by a user should have a file in this directory
	# bundles installed as dependencies should not have files here
	if [ "$track_bundle" = true ]; then
		sudo mkdir -p "$state_path"/bundles
		sudo touch "$state_path"/bundles/"$bundle_name"
	fi

	debug_msg "Bundle \"$bundle_name\" created successfully"

}

# Removes a bundle from the target-dir and/or the web-dir
# Parameters
# - -L: if this option is set the bundle is removed from the target-dir only,
#       otherwise it is removed from target-dir and web-dir
# - BUNDLE_MANIFEST: the manifest of the bundle to be removed
# - REPO_NAME: if the bundle is from a 3rd-party repo we need to specify the repo name
#              since these bundles get installed in a different location
remove_bundle() { # swupd_function

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    remove_bundle [-L] <bundle_manifest> [3rd-party repo name]

			Options:
			    -L   If set, the bundle will be removed from the target-dir only,
			         otherwise it is removed from both target-dir and web-dir
			EOM
		return
	fi
	local remove_local=false
	[ "$1" = "-L" ] && { remove_local=true ; shift ; }
	local bundle_manifest=$1
	local repo_name=$2
	local target_path
	local version_path
	local bundle_name
	local file_names
	local dir_names
	local manifest_file

	# if the bundle's manifest is not found just return
	if [ ! -e "$bundle_manifest" ]; then
		echo "$(basename "$bundle_manifest") not found, maybe the bundle was already removed"
		return
	fi

	if [ -z "$repo_name" ]; then
		target_path=$(dirname "$bundle_manifest" | cut -d "/" -f1)/testfs/target-dir
	else
		target_path=$(dirname "$bundle_manifest" | cut -d "/" -f1)/testfs/target-dir/opt/3rd_party/"$repo_name"
	fi
	version_path=$(dirname "$bundle_manifest")
	manifest_file=$(basename "$bundle_manifest")
	bundle_name=${manifest_file#Manifest.}

	# remove all files that are in the manifest from target-dir first
	mapfile -t file_names < <(awk '/^[FL]...\t/ { print $4 }' "$bundle_manifest")
	for fname in "${file_names[@]}"; do
		sudo rm -f "$target_path$fname"
	done
	# now remove all directories in the manifest (only if empty else they
	# may be used by another bundle)
	mapfile -t dir_names < <(awk '/^D...\t/ { print $4 }' "$bundle_manifest")
	for dname in "${dir_names[@]}"; do
		sudo rmdir --ignore-fail-on-non-empty "$target_path$dname" 2> /dev/null
	done
	if [ "$remove_local" = false ]; then
		# there is no need to remove the files and tars from web-dir/<ver>/files
		# as long as we remove the manifest from the bundle from all versions
		# where it shows up and from the MoM, the files may be used by another bundle
		sudo rm -f "$version_path"/"$manifest_file"
		sudo rm -f "$version_path"/"$manifest_file".tar
		# remove packs
		sudo rm "$version_path"/pack-"$bundle_name"-from-*.tar
		# finally remove it from the MoM
		remove_from_manifest "$version_path"/Manifest.MoM "$bundle_name"
	fi

}

# Installs a bundle in target-dir
# Parameters:
# - BUNDLE_MANIFEST: the manifest of the bundle to be installed

install_bundle() { # swupd_function

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
	target_path=$(dirname "$bundle_manifest" | cut -d "/" -f1)/testfs/target-dir
	files_path=$(dirname "$bundle_manifest")/files
	manifest_file=$(basename "$bundle_manifest")
	bundle_name=${manifest_file#Manifest.}

	# make sure the bundle is not already installed
	if [ -e "$target_path"/usr/share/clear/bundles/"$bundle_name" ]; then
		return
	fi

	# iterate through the manifest and copy all the files in its
	# correct place, start with directories
	mapfile -t dir_names < <(awk '/^D...\t/ { print $4 }' "$bundle_manifest")
	for dname in "${dir_names[@]}"; do
		sudo mkdir -p "$target_path$dname"
	done
	# now files
	mapfile -t file_names < <(awk '/^F...\t/ { print $4 }' "$bundle_manifest")
	for fname in "${file_names[@]}"; do
		fhash=$(get_hash_from_manifest "$bundle_manifest" "$fname")
		sudo cp "$files_path"/"$fhash" "$target_path$fname"
	done
	# finally links
	mapfile -t link_names < <(awk '/^L...\t/ { print $4 }' "$bundle_manifest")
	for lname in "${link_names[@]}"; do
		lhash=$(get_hash_from_manifest "$bundle_manifest" "$lname")
		fhash=$(readlink "$files_path"/"$lhash")
		# is the original link dangling?
		if [[ $fhash = *"does_not_exist"* ]]; then
			sudo ln -s "$fhash" "$target_path$lname"
		else
			fname=$(awk "/$fhash/ "'{ print $4 }' "$bundle_manifest")
			sudo ln -s "$(basename "$fname")" "$target_path$lname"
		fi
	done

}

# Updates one file or directory from a bundle, the update will be created in whatever version
# is the latest one (from web-dir/formatstaging/latest)
# Parameters:
# - -p: if the p (partial) flag is set the function skips updating the hashes
#       in the MoM, and re-creating the bundle's tar, this is useful if more changes are to be done
#       in order to reduce time
# - -i: if the i (iterative) flag is set the iterative manifests are created with every update
# - ENVIRONMENT_NAME: the name of the test environment
# - BUNDLE_NAME: the name of the bundle to be updated
# - OPTION: the kind of update to be performed { --add, --add-dir, --delete, --ghost, --rename, --rename-legacy, --update }
# - FILE_NAME: file or directory of the bundle to add or update
# - NEW_NAME: when --rename is chosen this parameter receives the new name to assign
update_bundle() { # swupd_function

	local partial=false
	[ "$1" = "-p" ] && { partial=true ; shift ; }
	local iterative=false
	[ "$1" = "-i" ] && { iterative=true ; shift ; }
	local env_name=$1
	local bundle=$2
	local option=$3
	local fname=$4
	local new_name=$5
	local version
	local version_path
	local oldversion
	local oldversion_path
	local bundle_manifest
	local fdir
	local new_dir
	local new_file
	local contentsize
	local fsize
	local fhash
	local fname
	local filename
	local new_fhash
	local new_fsize
	local new_fname
	local delta_name
	local format
	local to_manifest
	local to_manifest_content
	local from_manifest
	local from_manifest_content
	local pre_ver
	local pre_hash

	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --add <file_name>[:<path_to_existing_file>]
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --add-dir <directory_name>
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --delete <file_name>
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --ghost <file_name>
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --update <file_name>
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --rename[-legacy] <file_name> <new_name>
			    update_bundle [-p] [-i] <environment_name> <bundle_name> --header-only

			Options:
			    -p    If set (partial), the bundle will be updated, but the manifest's tar won't
			          be re-created nor the hash will be updated in the MoM. Use this flag when more
			          updates or changes will be done to the bundle to save time.
			    -i    If set (iterative), the iterative manifests will be created with every bundle
			          update.

			    NOTE: if both options -p and -i are to be used, they must be specified in that order or
			          one option will be ignored.

			EOM
		return
	fi

	if [ "$option" = "--header-only" ]; then
		fname="dummy"
	fi
	validate_path "$env_name"
	validate_param "$bundle"
	validate_param "$option"
	validate_param "$fname"

	# make sure fname starts with a slash
	if [[ "$fname" != "/"* ]]; then
		fname=/"$fname"
	fi

	# replace all the "/" in fname with "\/" so they are escaped (e.g. fname=/foo/bar, filename=\/foo\/bar)
	filename="${fname////\\/}"

	# collect usefule variables considering the version where
	# the update will be created is always the latest version
	version=$(get_latest_version "$env_name")
	version_path="$env_name"/web-dir/"$version"
	bundle_manifest="$version_path"/Manifest."$bundle"
	format=$(cat "$version_path"/format)

	# find the previous version of this bundle manifest
	oldversion=$(get_manifest_previous_version "$env_name" "$bundle" "$version")
	oldversion_path="$env_name"/web-dir/"$oldversion"

	# since we are going to be making updates to the bundle, copy its manifest
	# from the old version directory to the new one (if not copied already)
	copy_manifest "$env_name" "$bundle" "$oldversion" "$version"
	contentsize=$(awk '/^contentsize/ { print $2 }' "$bundle_manifest")

	# these actions apply to all operations that modify an existing file somehow
	if [ "$option" != "--add" ] && [ "$option" != "--add-dir" ] && [ "$option" != "--add-file" ] && [ "$option" != "--header-only" ]; then
		fhash=$(get_hash_from_manifest "$bundle_manifest" "$fname")
		fsize=$(stat -c "%s" "$oldversion_path"/files/"$fhash")
		# update the version of the file to be updated in the manifest
		update_manifest -p "$bundle_manifest" file-version "$fname" "$version"
	fi

	case "$option" in

	--add | --add-file)

		# if the directories the file is don't exist, add them to the bundle
		fdir=$(dirname "${fname%:*}")
		if ! sudo cat "$bundle_manifest" | grep -qx "D\\.\\.\\..*$fdir" && [ "$fdir" != "/" ]; then
			new_dir=$(create_dir "$version_path"/files)
			add_to_manifest -p "$bundle_manifest" "$new_dir" "$fdir"
			# add each one of the directories of the path if they are not in the manifest already
			while [ "$(dirname "$fdir")" != "/" ]; do
				fdir=$(dirname "$fdir")
				if ! sudo cat "$bundle_manifest" | grep -qx "D\\.\\.\\..*$fdir"; then
					add_to_manifest -p "$bundle_manifest" "$new_dir" "$fdir"
				fi
			done
			# Add the dir to the delta-pack
			add_to_pack "$bundle" "$new_dir" "$oldversion"
		fi

		# if the user wants to use an existing file, use it, else create a new one
		if [[ "$fname" = *":"* ]]; then
			new_file="${fname#*:}"
			validate_item "$new_file"
			new_fhash=$(sudo "$SWUPD" hashdump --quiet "$new_file")
			sudo rsync -aq "$new_file" "$version_path"/files/"$new_fhash"
			if [ ! -e "$version_path"/files/"$new_fhash".tar ]; then
				create_tar "$version_path"/files/"$new_fhash"
			fi
			new_file="$version_path"/files/"$new_fhash"
			fname="${fname%:*}"
		else
			new_file=$(create_file "$version_path"/files)
		fi
		add_to_manifest -p "$bundle_manifest" "$new_file" "$fname"

		# contentsize is automatically added by the add_to_manifest function so
		# all we need is to get the updated value for now
		contentsize=$(awk '/^contentsize/ { print $2 }' "$bundle_manifest")

		# Add the file to the zero pack of the bundle
		add_to_pack "$bundle" "$new_file"

		# Add the new file to all previous delta-packs
		pre_ver="$version"
		while [ "$pre_ver" -ge 0 ]; do
			pre_ver=$(get_manifest_previous_version "$env_name" MoM "$pre_ver")
			if [ "$pre_ver" -eq 0 ]; then
				break
			fi
			add_to_pack "$bundle" "$new_file" "$pre_ver"
		done
		;;

	--add-dir)

		# if the directories the file is don't exist, add them to the bundle
		fdir="$fname"
		if ! sudo cat "$bundle_manifest" | grep -qx "D\\.\\.\\..*$fdir" && [ "$fdir" != "/" ]; then
			new_dir=$(create_dir "$version_path"/files)
			add_to_manifest -p "$bundle_manifest" "$new_dir" "$fdir"
			# add each one of the directories of the path if they are not in the manifest already
			while [ "$(dirname "$fdir")" != "/" ]; do
				fdir=$(dirname "$fdir")
				if ! sudo cat "$bundle_manifest" | grep -qx "D\\.\\.\\..*$fdir"; then
					add_to_manifest -p "$bundle_manifest" "$new_dir" "$fdir"
				fi
			done

			# since directories usually have the same hash, add it to
			# the zero pack only if it doesn't already have one
			if ! tar -tf "$version_path"/pack-"$bundle"-from-0.tar | grep -q "${new_dir##*/}"; then
				add_to_pack "$bundle" "$new_dir"
			fi

			# Add the dir to all previous delta-packs if not already there
			pre_ver="$version"
			while [ "$pre_ver" -ge 0 ]; do
				pre_ver=$(get_manifest_previous_version "$env_name" MoM "$pre_ver")
				if [ "$pre_ver" -eq 0 ]; then
					break
				fi
				if [ -e "$version_path"/pack-"$bundle"-from-"$pre_ver".tar ]; then
					if tar -tf "$version_path"/pack-"$bundle"-from-"$pre_ver".tar | grep -q "${new_dir##*/}"; then
						continue
					fi
				fi
				add_to_pack "$bundle" "$new_dir" "$pre_ver"
			done

		fi

		contentsize=$(awk '/^contentsize/ { print $2 }' "$bundle_manifest")
		;;

	--delete | --ghost)

		# replace the first character of the line that matches with "."
		sudo sed -i "/\\t$filename$/s/./\\./1" "$bundle_manifest"
		sudo sed -i "/\\t$filename\\t/s/./\\./1" "$bundle_manifest"
		if [ "$option" = "--delete" ]; then
			# replace the second character of the line that matches with "d"
			sudo sed -i "/\\t$filename$/s/./d/2" "$bundle_manifest"
			sudo sed -i "/\\t$filename\\t/s/./d/2" "$bundle_manifest"
			# remove the related file(s) from the version dir (if there)
			sudo rm -f "$version_path"/files/"$fhash"
			sudo rm -f "$version_path"/files/"$fhash".tar
		else
			# replace the second character of the line that matches with "g"
			sudo sed -i "/\\t$filename$/s/./g/2" "$bundle_manifest"
			sudo sed -i "/\\t$filename\\t/s/./g/2" "$bundle_manifest"
		fi

		# replace the hash with 0s
		update_manifest -p "$bundle_manifest" file-hash "$fname" "$zero_hash"

		# calculate new contentsize (NOTE: filecount is not decreased)
		contentsize=$((contentsize - fsize))
		;;

	--update)

		# append random content to the file
		generate_random_content 1 20 | sudo tee -a "$version_path"/files/"$fhash" > /dev/null

		# recalculate hash and update file names
		new_fhash=$(sudo "$SWUPD" hashdump --quiet "$version_path"/files/"$fhash")
		sudo mv "$version_path"/files/"$fhash" "$version_path"/files/"$new_fhash"
		create_tar "$version_path"/files/"$new_fhash"
		sudo rm -f "$version_path"/files/"$fhash".tar

		# update the manifest with the new hash
		update_manifest -p "$bundle_manifest" file-hash "$fname" "$new_fhash"

		# calculate new contentsize
		new_fsize=$(stat -c "%s" "$version_path"/files/"$new_fhash")
		contentsize=$((contentsize + (new_fsize - fsize)))

		# update the zero-pack with the new file (leave the old file, it doesn't matter)
		add_to_pack "$bundle" "$version_path"/files/"$new_fhash"

		# create delta packs for all previous versions
		pre_ver="$version"
		while [ "$pre_ver" -ge 0 ]; do

			pre_ver=$(get_manifest_previous_version "$env_name" MoM "$pre_ver")
			if [ "$pre_ver" -eq 0 ]; then
				break
			fi

			# if the manifest doesn't exist in that version, but exists in the MoM
			# the bundle didn't go through any changes there, copy if from previous version
			if [ ! -e "$env_name"/web-dir/"$pre_ver"/Manifest."$bundle" ]; then
				local pv
				pv=$(awk "/M\\.\\.\\.\\t.*\\t.*\\t$bundle/"'{ print $3 }' "$env_name"/web-dir/"$pre_ver"/Manifest.MoM)
				if [ -z "$pv" ]; then
					# the bundle didn't exist in earlier versions, break from the loop
					break
				fi
				copy_manifest "$env_name" "$bundle" "$pv" "$pre_ver"
			fi

			# create the delta file
			pre_hash=$(get_hash_from_manifest "$env_name"/web-dir/"$pre_ver"/Manifest."$bundle" "$fname")
			# if the file didn't exist in that version we need to add the whole file
			# not just the delta
			if [ -z "$pre_hash" ]; then
				add_to_pack "$bundle" "$version_path"/files/"$new_fhash" "$pre_ver"
				continue
			fi
			delta_name="$pre_ver-$version-$pre_hash-$new_fhash"
			sudo bsdiff "$env_name"/web-dir/"$pre_ver"/files/"$pre_hash" "$version_path"/files/"$new_fhash" "$version_path"/delta/"$delta_name"

			# create or add to the delta-pack
			add_to_pack "$bundle" "$version_path"/delta/"$delta_name" "$pre_ver"

		done

		# keep the modified file for the iterative "from manifest"
		from_manifest_content="$(awk "/....\\t.*\\t.*\\t$filename/" "$oldversion_path"/Manifest."$bundle")"$'\n'
		;;

	--rename | --rename-legacy)

		validate_param "$new_name"
		# make sure new_name starts with a slash
		if [[ "$new_name" != "/"* ]]; then
			new_name=/"$new_name"
		fi
		new_fname="${new_name////\\/}"

		# renames need two records in the manifest, one with the
		# new name (F...) and one with the old one (.d..)
		# replace the first character of the old record with "."
		sudo sed -i "/\\t$filename$/s/./\\./1" "$bundle_manifest"
		sudo sed -i "/\\t$filename\\t/s/./\\./1" "$bundle_manifest"
		# replace the second character of the old record with "d"
		sudo sed -i "/\\t$filename$/s/./d/2" "$bundle_manifest"
		sudo sed -i "/\\t$filename\\t/s/./d/2" "$bundle_manifest"

		# add the new name to the manifest
		add_to_manifest -p "$bundle_manifest" "$oldversion_path"/files/"$fhash" "$new_name"
		if [ "$option" = "--rename" ]; then
			# replace the hash of the old record with 0s
			update_manifest -p "$bundle_manifest" file-hash "$fname" "$zero_hash"
		else
			# replace the fourth character of the old record with "r"
			sudo sed -i "/\\t$filename$/s/./r/4" "$bundle_manifest"
			sudo sed -i "/\\t$filename\\t/s/./r/4" "$bundle_manifest"
			# replace the fourth character of the new record with "r"
			sudo sed -i "/\\t$new_fname$/s/./r/4" "$bundle_manifest"
		fi

		pre_ver="$version"
		while [ "$pre_ver" -ge 0 ]; do

			pre_ver=$(get_manifest_previous_version "$env_name" MoM "$pre_ver")
			if [ "$pre_ver" -eq 0 ]; then
				break
			fi

			# if the manifest doesn't exist in that version, but exists in the MoM
			# the bundle didn't go through any changes there, copy if from previous version
			if [ ! -e "$env_name"/web-dir/"$pre_ver"/Manifest."$bundle" ]; then
				local pv
				pv=$(awk "/M\\.\\.\\.\\t.*\\t.*\\t$bundle/"'{ print $3 }' "$env_name"/web-dir/"$pre_ver"/Manifest.MoM)
				if [ -z "$pv" ]; then
					# the bundle didn't exist in earlier versions, break from the loop
					break
				fi
				copy_manifest "$env_name" "$bundle" "$pv" "$pre_ver"
			fi

			# if the file didn't exist in that version we need to add the whole file
			# not just the delta
			if ! grep -q "$fhash" "$env_name"/web-dir/"$pre_ver"/Manifest."$bundle"; then
				add_to_pack "$bundle" "$version_path"/files/"$fhash" "$pre_ver"
				continue
			fi

			# create the delta file
			delta_name="$pre_ver-$version-$fhash-$fhash"
			sudo bsdiff "$env_name"/web-dir/"$pre_ver"/files/"$fhash" "$version_path"/files/"$fhash" "$version_path"/delta/"$delta_name"

			# create or add to the delta-pack
			add_to_pack "$bundle" "$version_path"/delta/"$delta_name" "$pre_ver"

		done

		;;

	--header-only)

		# do nothing
		;;

	*)
		terminate "Please select a valid option for updating the bundle: --add, --delete, --ghost, --rename, --update, --header-only"
		;;

	esac

	# re-order items on the manifest so they are in the correct order based on version
	sudo sort -t$'\t' -k3 -s -h -o "$bundle_manifest" "$bundle_manifest"
	update_manifest -p "$bundle_manifest" contentsize "$contentsize"
	update_manifest -p "$bundle_manifest" timestamp "$(date +"%s")"

	if [ "$iterative" = true ]; then
		# create the iterative "to manifest"
		to_manifest="$bundle_manifest".I."$oldversion"
		# get the manifest header as is
		to_manifest_content=$(sed "/^[^\\t]*\\t[^\\t]*$/! d" "$bundle_manifest")$'\n'$'\n'
		# get only those bundles that match the latest version
		to_manifest_content+=$(sed "/^....\\t.*\\t$version\\t.*$/! d" "$bundle_manifest")$'\n'
		write_to_protected_file "$to_manifest" "$to_manifest_content"
		update_manifest "$to_manifest" filecount "$(grep -c -E "^[FDL]..." "$to_manifest")"
		# TODO(castulo): update the contentsize in the to_manifest
		# update the MoM, first remove any other iterative manifest for that bundle if one exists
		if [ -n "$(awk "/$bundle.I.*/"'{ print $4 }' "$version_path"/Manifest.MoM)" ]; then
			remove_from_manifest -p "$version_path"/Manifest.MoM "$(awk "/$bundle.I.*/"'{ print $4 }' "$version_path"/Manifest.MoM)"
		fi
		# now add the iterative manifest
		add_to_manifest -p "$version_path"/Manifest.MoM "$to_manifest" "$bundle".I."$oldversion"

		# create the iterative "from manifest" (if there are deltas)
		if [ -n "$from_manifest_content" ]; then
			from_manifest="$version_path"/"$version"/Manifest."$bundle".D."$oldversion"
			if [ ! -e "$from_manifest" ]; then
				sudo mkdir -p "$version_path"/"$version"
				write_to_protected_file "$from_manifest" "$(sed "/^[^\\t]*\\t[^\\t]*$/! d" "$oldversion_path"/Manifest."$bundle")"$'\n'$'\n'
			fi
			write_to_protected_file -a "$from_manifest" "$from_manifest_content"
			update_manifest -p "$from_manifest" filecount "$(grep -c -E "^(F|D)..." "$from_manifest")"
			update_manifest -p "$from_manifest" timestamp "$(date +"%s")"
			# TODO(castulo): update the contentsize in the from_manifest
			add_to_pack "$bundle" "$from_manifest" "$oldversion"
		fi
	fi

	# renew the manifest tar
	if [ "$partial" = false ]; then
		sudo rm -f "$bundle_manifest".tar
		create_tar "$bundle_manifest"
		# update the mom
		update_hashes_in_mom "$version_path"/Manifest.MoM
	fi

}

# Creates a manifest delta file for one bundle
# is the latest one (from web-dir/formatstaging/latest)
# Parameters:
# - BUNDLE_NAME: the name of the bundle to be updated
# - VERSION: the version of the server side content
# - FROM_VERSION: the previous version, if nothing is selected defaults to 0
create_delta_manifest() { # swupd_function
	local bundle=$1
	local version=$2
	local from_version=$3
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    create_delta_manifest <bundle_name> <item> [from_version]
			EOM
		return
	fi

	sudo bsdiff "$WEBDIR/$from_version/Manifest.$bundle" "$WEBDIR/$version/Manifest.$bundle" "$WEBDIR/$version/Manifest-$bundle-delta-from-$from_version"
}

# Adds the specified file to the zero or delta pack for the bundle
# Parameters:
# - BUNDLE: the name of the bundle
# - ITEM: the file or directory to be added into the pack
# - FROM_VERSION: the from version for the pack, if not specified a zero pack is asumed
add_to_pack() { # swupd_function

	local bundle=$1
	local item=$2
	local version=${3:-0}
	local item_path
	local version_path
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    add_to_pack <bundle_name> <item> [from_version]
			EOM
		return
	fi

	item_path=$(dirname "$item")
	version_path=$(dirname "$item_path")

	# item should be a file from the "delta" or "files" directories
	# fullfiles are expected to be in the staged dir when extracted
	if [[ "$item" = *"/files"* ]]; then
		sudo tar -C "$item_path" -rf "$version_path"/pack-"$bundle"-from-"$version".tar --transform "s,^,staged/," "$(basename "$item")"
	elif [[ "$item" = *"/delta"* ]]; then
		sudo tar -C "$version_path" -rf "$version_path"/pack-"$bundle"-from-"$version".tar delta/"$(basename "$item")"
	elif [[ "$item" = *"/Manifest."*".D"* ]]; then
		sudo tar -C "$version_path" -rf "$version_path"/pack-"$bundle"-from-"$version".tar "$(basename "$version_path")"/"$(basename "$item")"
	else
		terminate "the provided file is not valid in a zero pack"
	fi

}

# Cleans up the directories in the state dir
# Parameters:
# - ENV_NAME: the name of the test environment to have the state dir cleaned up
# - REPO_NAME: if the state dir belongs to 3rd-party repo, the name should be specified here
clean_state_dir() { # swupd_function

	local env_name=$1
	local repo_name=$2
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    clean_state_dir <environment_name> [3rd-party repo]
			EOM
		return
	fi
	validate_path "$env_name"
	if [ -z "$repo_name" ]; then
		sudo rm -rf "$env_name"/testfs/state
		sudo mkdir -p "$env_name"/testfs/state/{staged,download,delta,telemetry}
	else
		sudo rm -rf "$env_name"/testfs/state/3rd_party/"$repo_name"
		sudo mkdir -p "$env_name"/testfs/state/3rd_party/"$repo_name"/{staged,download,delta,telemetry,bundles}
	fi
	sudo chmod -R 0700 "$env_name"/testfs/state

}

# Creates a new test case based on a template
# Parameters:
# - NAME: the name (and path) of the test to be generated
generate_test() { # swupd_function

	local name=$1
	local path
	local id
	local git_name
	local git_email

	# If no parameters are received show usage
	usage () {
		cat <<-EOM
			Usage:
			    generate_test [relative_path]/<test_name>
			EOM
	}
	if [ $# -eq 0 ]; then
		usage
		return
	fi
	validate_param "$name"

	path=$(dirname "$(realpath "$name")")
	if [[ "$path" != *swupd-client/test/functional/* ]]; then
		echo -e "All functional tests should be grouped within a directory in swupd-client/test/functional/<group_dir>/\\n"
		usage
		return 1
	fi
	name=$(basename "$name")
	id=$(get_next_available_id "$path")
	if [ $? == 1 ]; then
		id="<test ID>"
	fi
	git_name=$(git config -l | grep user.name | sed 's/user.name=//')
	if [ -z "$git_name" ]; then
		git_name="<author name>"
	fi
	git_email=$(git config -l | grep user.email | sed 's/user.email=//')
	if [ -z "$git_email" ]; then
		git_email="<author email>"
	fi

	{
		printf '#!/usr/bin/env bats\n\n'
		printf '# Author: %s\n' "$git_name"
		printf '# Email: %s\n\n' "$git_email"
		printf 'load "../testlib"\n\n'
		printf 'global_setup() {\n\n'
		printf '\t# global setup\n\n'
		printf '}\n\n'
		printf 'test_setup() {\n\n'
		# shellcheck disable=SC2016
		printf '\t# create_test_environment "$TEST_NAME"\n'
		# shellcheck disable=SC2016
		printf '\t# create_bundle -n <bundle_name> -f <file_1>,<file_2>,<file_N> "$TEST_NAME"\n\n'
		printf '}\n\n'
		printf 'test_teardown() {\n\n'
		# shellcheck disable=SC2016
		printf '\t# destroy_test_environment "$TEST_NAME"\n\n'
		printf '}\n\n'
		printf 'global_teardown() {\n\n'
		printf '\t# global cleanup\n\n'
		printf '}\n\n'
		printf '@test "%s: <test description>" {\n\n' "$id"
		printf '\t# <If necessary add a detailed explanation of the test here>\n\n'
		# shellcheck disable=SC2016
		printf '\trun sudo sh -c "$SWUPD <swupd_command> $SWUPD_OPTS <command_options>"\n\n'
		# shellcheck disable=SC2016
		printf '\t# assert_status_is "$SWUPD_OK"\n'
		# shellcheck disable=SC2016
		printf '\t# expected_output=$(cat <<-EOM\n'
		printf '\t# \t<expected output>\n'
		printf '\t# EOM\n'
		printf '\t# )\n'
		# shellcheck disable=SC2016
		printf '\t# assert_is_output "$expected_output"\n\n'
		printf '}\n'
	} > "$path"/"$name".bats
	# make the test script executable
	chmod +x "$path"/"$name".bats

}

# Gets the next available ID for the specified test group
# Parameters:
# - GROUP_DIRECTORY: the path to the directory of the test group
get_next_available_id() { # swupd_function

	local test_dir=$1
	local id=0
	local group
	local test_list
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    get_next_available_id <test_directory>
			EOM
		return
	fi
	validate_path "$test_dir"

	# shellcheck disable=SC2126
	id=$(grep -r --include="*.bats" "@test .* {" "$test_dir" | wc -l)
	id=$((id+1))
	test_dir=$(basename "$(realpath "$test_dir")")
	case "$test_dir" in
		3rd-party) group=TPR;;
		autoupdate) group=AUT;;
		bundleadd) group=ADD;;
		bundleremove) group=REM;;
		bundlelist) group=LST;;
		bundleinfo) group=BIN;;
		diagnose) group=DIA;;
		update) group=UPD;;
		checkupdate) group=CHK;;
		search) group=SRH;;
		hashdump) group=HSD;;
		mirror) group=MIR;;
		completion) group=USA;;
		usability) group=USA;;
		signature) group=SIG;;
		info) group=INF;;
		clean) group=CLN;;
		os-install) group=INS;;
		repair) group=REP;;
		verify-legacy) group=VER;;
		*) group=UNKNOWN;;
	esac
	id=$(printf "$group%03d\\n" $id)
	test_list="$(list_tests "$test_dir")"
	# if the next available ID is not really available it means the list is messed up
	if [ "${test_list/$id}" != "$test_list" ]; then
		echo -e "There is a problem with the current IDs, one ID seems to be missing from the list:\\n"
		echo "$test_list"
		echo -e "\\nPlease fix the IDs as appropriate and try running the command again\\n"
		return 1
	else
		echo "$id"
	fi

}

# Prints the list of tests in a directory
# Parameters:
# - GROUP_DIRECTORY: the path to the directory of the test group
list_tests() { # swupd_function

	local test_dir=$1
	# If no parameters are received show usage
	if [ $# -eq 0 ]; then
		cat <<-EOM
			Usage:
			    list_tests --all
			    list_tests <test_directory>
			EOM
		return
	fi

	if [ "$test_dir" = --all ]; then
		grep -rh --include="*.bats" "@test .* {" "$FUNC_DIR" | sed "s/@test \"//" | sed "s/\" {.*//" | sort
	else
		if [[ "$(dirname "$(realpath "$test_dir")")" != *swupd-client/test/functional ]]; then
			echo "Invalid test group directory"
			return 1
		fi
		ls "$test_dir"/*.bats >/dev/null 2>&1 || return 0
		grep -rh --include="*.bats" "@test .* {" "$test_dir" | sed "s/@test \"//" | sed "s/\" {.*//" | sort
	fi

}

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# The section below contains test fixtures that can be used from tests to create and
# cleanup test dependencies, these functions can be overwritten in the test script.
# The intention of these is to try reducing the amount of boilerplate included in
# tests since all tests require at least the creation of a  test environment
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

setup() {

	TESTLIB_WD="$(pwd)"
	export TESTLIB_WD

	print "\\n\\n\\n$sep"
	print "$alt_sep"
	print "$sep\\n"

	debug_msg "Test file: $TEST_NAME.bats"
	debug_msg "BATS test number: $BATS_TEST_NUMBER"

	# run the global_setup only once
	if [ "$BATS_TEST_NAME" = "${BATS_TEST_NAMES[0]}" ]; then

		# if a global environment exists at this point it means
		# it was a leftover from a previous execution, remove it
		if [ -d "$TEST_NAME" ]; then
			debug_msg "\\nAn old test environment was found for this test: $TEST_NAME"
			debug_msg "Deleting it..."
			destroy_test_environment --force "$TEST_NAME"
		fi

		debug_msg "\\nRunning global setup..."
		global_setup
		debug_msg "Global setup finished\\n"

		# check if we created a test environment in the global_setup
		if [ -d "$TEST_NAME" ]; then
			sudo touch "$TEST_NAME"/.global_env
			print "Global test environment created: $TEST_NAME"
		fi

	fi

	if [ -e "$TEST_NAME"/.global_env ]; then
		debug_msg "\\nSetting environment variables for the current test..."
		set_env_variables "$TEST_NAME"
	else
		# individual environments will be used for each test,
		# we need to name the env differently for each test
		if ! [[ $BATS_TEST_DESCRIPTION =~ ^[a-zA-Z]{3}[0-9]{3}":"* ]]; then
			terminate "Bad test, missing test ID: \"$BATS_TEST_DESCRIPTION\""
		fi
		TEST_NAME="$TEST_NAME"_"${BATS_TEST_DESCRIPTION%:*}"

		# if a local environment exists at this point it is a
		# leftover from a previous execution, remove it
		if [ -d "$TEST_NAME" ]; then
			debug_msg "\\nAn old test environment was found for this test: $TEST_NAME"
			debug_msg "Deleting it..."
			destroy_test_environment --force "$TEST_NAME"
		fi
	fi

	debug_msg "\\nTEST_NAME: $TEST_NAME"
	debug_msg "Test path: $(pwd)/$TEST_NAME"

	# now run the test setup
	debug_msg "\\nRunning test_setup..."
	test_setup
	debug_msg "Finished running test_setup\\n"

	print "Test setup complete. Starting test execution..."

}

teardown() {

	# make sure to go back to the original working directory if the user
	# changed it during a test
	if [ "$(pwd)" != "$TESTLIB_WD" ]; then
		cd "$TESTLIB_WD" || terminate "there was an error going back to the original working directory"
	fi

	local index
	# if the user wants to preserve the output and we are using an global environment
	# we need to copy the state of the environment before starting the next test or
	# the state will be overrriden
	if [ "$KEEP_ENV" = true ] && [ -e "$TEST_NAME"/.global_env ] && [ -d "$TEST_NAME" ]; then
		print "Saving a copy of the state dir in $TEST_NAME/state_${BATS_TEST_DESCRIPTION%:*}"
		sudo cp -r "$TEST_NAME"/testfs/state "$TEST_NAME"/state_"${BATS_TEST_DESCRIPTION%:*}"
	fi

	debug_msg "\\nRunning test_teardown..."
	test_teardown
	debug_msg "Finished running test_teardown\\n"

	# if the last test just ran, run the global teardown
	index=$(( ${#BATS_TEST_NAMES[*]} - 1 ))
	if [ "$BATS_TEST_NAME" = "${BATS_TEST_NAMES[$index]}" ]; then

		debug_msg "\\nRunning global teardown..."
		global_teardown
		debug_msg "Global teardown finished\\n"

	fi

	print "Test teardown complete."

}

global_setup() {

	# if global_setup is not defined it will default to this one
	debug_msg "No global setup was defined"

}

global_teardown() {

	# if global_teardown is not defined it will default to this one
	debug_msg "No global teardown was defined"

}

# Default test_setup
test_setup() {

	debug_msg "No test_setup was defined, using the default one..."
	create_test_environment "$TEST_NAME"

}

# Default test_teardown
test_teardown() {

	debug_msg "No test_teardown was defined, using the default one..."
	destroy_test_environment "$TEST_NAME"

}

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# The section below contains functions useful for consistent test validation and output
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sep="------------------------------------------------------------------"
alt_sep="++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

print_assert_failure() {

	local message=$1
	validate_param "$message"

	echo -e "\\nAssertion Failed"
	echo -e "$message"
	echo "Command output:"
	echo "------------------------------------------------------------------"
	# shellcheck disable=SC2154
	# SC2154: var is referenced but not assigned
	# the output variable is being assigned and exported by bats
	echo "$output"
	echo "------------------------------------------------------------------"

}

use_ignore_list() {

	local ignore_enabled=$1
	local filtered_output
	validate_param "$ignore_enabled"

	# if selected, remove things in the ignore list from the actual output
	if [ "$ignore_enabled" = true ]; then
		# always remove blank lines, lines with only dots and lines with progress percentage
		filtered_output=$(echo "$output" | sed -E '/^$/d' | sed -E '/^\.+$/d' | sed -E '/^\t\.\.\.[0-9]{1,3}%$/d')
		# now remove lines that are included in any of the ignore-lists
		# there are 3 possible ignore-lists that the function is going
		# to recognize (in order of precedence):
		# - <functional_tests_directory>/<test_theme_directory>/<test_name>.ignore-list
		# - <functional_tests_directory>/<test_theme_directory>/ignore-list
		# - <functional_tests_directory>/ignore-list
		if [ -f "$THEME_DIRNAME"/"$TEST_NAME_SHORT".ignore-list ]; then
			ignore_list="$THEME_DIRNAME"/"$TEST_NAME_SHORT".ignore-list
		elif [ -f "$THEME_DIRNAME"/ignore-list ]; then
			ignore_list="$THEME_DIRNAME"/ignore-list
		elif [ -f "$FUNC_DIR"/ignore-list ]; then
			ignore_list="$FUNC_DIR"/ignore-list
		fi
		debug_msg "Filtering output using ignore file $ignore_list"
		while IFS= read -r line; do
			# if the pattern from the file has a "/" escape it first so it does
			# not confuses the sed command
			line="${line////\\/}"
			filtered_output=$(echo "$filtered_output" | sed -E "/^$line$/d")
		done < "$ignore_list"
	else
		debug_msg "The use of ignore lists is disabled"
		filtered_output="$output"
	fi
	echo "$filtered_output"

}

assert_status_is() { # assertion

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

assert_status_is_not() { # assertion

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

assert_dir_exists() { # assertion

	local vdir=$1
	validate_param "$vdir"

	if sudo test ! -d "$vdir"; then
		print_assert_failure "Directory $vdir should exist, but it does not"
		return 1
	fi

}

assert_dir_not_exists() { # assertion

	local vdir=$1
	validate_param "$vdir"

	if sudo test -d "$vdir"; then
		print_assert_failure "Directory $vdir should not exist, but it does"
		return 1
	fi

}

assert_file_exists() { # assertion

	local vfile=$1
	validate_param "$vfile"

	if sudo test ! -f "$vfile"; then
		print_assert_failure "File $vfile should exist, but it does not"
		return 1
	fi

}

assert_file_not_exists() { # assertion

	local vfile=$1
	validate_param "$vfile"

	if sudo test -f "$vfile"; then
		print_assert_failure "File $vfile should not exist, but it does"
		return 1
	fi

}

assert_in_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_not_in_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_is_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_is_not_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_regex_in_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_regex_not_in_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_regex_is_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_regex_is_not_output() { # assertion

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
		diff -u <(echo "$expected_output") <(echo "$actual_output")
		return 1
	fi

}

assert_equal() { # assertion

	local val1=$1
	local val2=$2
	validate_param "$val1"
	validate_param "$val2"

	if [ "$val1" != "$val2" ]; then
		return 1
	fi

}

assert_not_equal() { # assertion

	local val1=$1
	local val2=$2
	validate_param "$val1"
	validate_param "$val2"

	if [ "$val1" = "$val2" ]; then
		return 1
	fi

}

assert_files_equal() { # assertion

	local val1=$1
	local val2=$2
	validate_item "$val1"
	validate_item "$val2"

	diff -q "$val1" "$val2"

}

assert_files_not_equal() { # assertion

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

# Displays a menu of functions that can be used from within a test. Functions considered
# "internal" are not displayed.
# Functions are not included in the list by default, to mark a function to be included in
# the help menu, add a comment immediatelly after the function opening bracket with either
# the keyword swupd_function or assertion depending on the type of function.
# Parameters:
# - keyword: a word that could be used to filter functions
testlib() {

	local keyword=$1
	local funcs
	local assertions

	funcs=$(grep "$keyword.*() { [#] swupd_function" "$FUNC_DIR"/testlib.bash | cut -f1 -d "(")
	assertions=$(grep "$keyword.*() { [#] assertion" "$FUNC_DIR"/testlib.bash | cut -f1 -d "(")

	echo ""
	if [ -n "$funcs" ]; then
		echo "------------------------------------------------------"
		echo "|    Functions provided by the SWUPD test library    |"
		echo "------------------------------------------------------"
		if [ -n "$keyword" ]; then
			echo -e "\\nTestlib functions with the '$keyword' keyword:\\n"
		fi
		echo "$funcs" | sort
	fi
	echo ""

	if [ -n "$assertions" ]; then
		echo ""
		echo "------------------------------------------------------"
		echo "|                     Assertions                     |"
		echo "------------------------------------------------------"
		if [ -n "$keyword" ]; then
			echo -e "\\nAssertions with the '$keyword' keyword:\\n"
		fi
		echo "$assertions"
	fi
	echo ""

}
