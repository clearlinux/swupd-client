#!bin/bash

FUNC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_FILENAME=$(basename "$BATS_TEST_FILENAME")
TEST_NAME=${TEST_FILENAME%.bats}
export TEST_NAME
export FUNC_DIR
export SWUPD_DIR="$FUNC_DIR/../.."
export SWUPD="$SWUPD_DIR/swupd"


generate_random_content() { 

	local bottom_range=5
	local top_range=100
	local range=$((top_range - bottom_range + 1))
	local number_of_lines=$((RANDOM%range))  
	< /dev/urandom tr -dc 'a-zA-Z0-9-_!@#$%^&*()_+{}|:<>?=' | fold -w 100 | head -n $number_of_lines

}

generate_random_name() { 

	local prefix=${1:-test-}
	local uuid
	
	# generate random 8 character alphanumeric string (lowercase only)
	uuid=$(< /dev/urandom tr -dc 'a-f0-9' | fold -w 8 | head -n 1)
	echo "$prefix$uuid"

}

validate_path() { 

	local path=$1
	if [ -z "$path" ] || [ ! -d "$path" ]; then
		echo "Please provide a valid path"
		return 1
	fi

}

validate_item() { 

	local vfile=$1
	if [ -z "$vfile" ] || [ ! -e "$vfile" ]; then
		echo "Please provide a valid file"
		return 1
	fi

}

validate_param() {

	local param=$1
	if [ -z "$param" ]; then
		echo "Mandatory parameter missing"
		return 1
	fi

}

# Exports environment variables that are dependent on the test environment
# Parameters:
# - ENV_NAME: the name of the test environment
set_env_variables() {

	local env_name=$1
	validate_path "$env_name"

	export SWUPD_OPTS="-S$BATS_TEST_DIRNAME/$env_name/state -p $BATS_TEST_DIRNAME/$env_name/target-dir -F staging -u file://$BATS_TEST_DIRNAME/$env_name/web-dir -C $FUNC_DIR/Swupd_Root.pem -I"

}

# Creates a directory with a hashed name in the specified path, if a directory
# already exists it returns the name
# Parameters:
# - PATH: the path where the directory will be created 
create_dir() { 
	
	local path=$1
	local hashed_name
	local directory
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
# Parameters:
# - PATH: the path where the symbolic link will be created
# - FILE: the path to the file to point to 
create_link() { 

	local path=$1
	local pfile=$2
	local hashed_name
	validate_path "$path"
	
	# if no file is specified, create one
	if [ -z "$pfile" ]; then
		pfile=$(create_file "$path")
	else
		validate_item "$pfile"
	fi
	sudo ln -rs "$pfile" "$path"/testlink
	hashed_name=$(sudo "$SWUPD" hashdump "$path"/testlink 2> /dev/null)
	sudo mv "$path"/testlink "$path"/"$hashed_name"
	echo "$path/$hashed_name"

}

# Creates a tar for the specified item in the same location
# Parameters:
# - ITEM: the relative path to the item (file, directory, link, manifest)
create_tar() {

	local item=$1
	local path
	local item_name
	validate_item "$item"

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
	validate_path "$path"
	validate_param "$name"

	version=$(basename "$path")	
	echo -e "MANIFEST\\t3" | sudo tee "$path"/Manifest."$name" > /dev/null
	echo -e "version:\\t$version" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo -e "previous:\\t0" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo -e "filecount:\\t0" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo -e "timestamp:\\t$(date +"%s")" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo -e "contentsize:\\t0" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo "" | sudo tee -a "$path"/Manifest."$name" > /dev/null
	echo "$path/Manifest.$name"
	
}

# Adds the specified item to an existing bundle manifest
# Parameters:
# - MANIFEST: the relative path to the manifest file
# - ITEM: the relative path to the item (file, directory, symlink) to be added
# - ITEM_PATH: the absolute path of the item in the target system when installed
add_to_manifest() { 

	local manifest=$1
	local item=$2
	local item_path=$3
	local item_type
	local item_size
	local name
	local version
	local filecount
	local contentsize
	local linked_file
	validate_item "$manifest"
	validate_item "$item"
	validate_param "$item_path"
	
	item_size=$(du -b "$item" | cut -f1)
	name=$(basename "$item")
	version=$(basename "$(dirname "$manifest")")
	# add to filecount
	filecount=$(sudo cat "$manifest" | grep filecount | awk '{ print $2 }')
	filecount=$((filecount + 1))
	sudo sed -i "s/filecount:.*/filecount:\\t$filecount/" "$manifest"
	# add to contentsize 
	contentsize=$(sudo cat "$manifest" | grep contentsize | awk '{ print $2 }')
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
		# its associated file too
		linked_file=$(readlink "$item")
		if [ ! "$(sudo cat "$manifest" | grep "$linked_file")" ]; then
			add_to_manifest "$manifest" "$(dirname "$item")"/"$linked_file" "$(dirname "$item_path")"/"$(generate_random_name test-file-)"
		fi
	elif [ -f "$item" ]; then
		item_type=F
	elif [ -d "$item" ]; then
		item_type=D
	fi
	sudo sed -i "s/contentsize:.*/contentsize:\\t$contentsize/" "$manifest"
	# add to manifest content
	echo -e "$item_type...\\t$name\\t$version\\t$item_path" | sudo tee -a "$manifest" > /dev/null
	
}

# Signs a manifest with a PEM key and generates the signed manifest in the same location
# Parameters:
# - MANIFEST: the path to the manifest to be signed
sign_manifest() {

	local manifest=$1
	validate_item "$manifest"

	sudo openssl smime -sign -binary -in "$manifest" \
    -signer "$FUNC_DIR"/Swupd_Root.pem \
    -inkey "$FUNC_DIR"/private.pem \
    -outform DER -out "$(dirname "$manifest")"/Manifest.MoM.sig
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
	echo "$version" | sudo tee "$env_name"/web-dir/version/formatstaging/latest > /dev/null
	sudo mkdir -p "$env_name"/web-dir/"$version"/files
	sudo mkdir -p "$env_name"/web-dir/"$version"/staged
	mom=$(create_manifest "$env_name"/web-dir/"$version" MoM)
	sign_manifest "$mom"

	# target-dir files & dirs
	sudo mkdir -p "$env_name"/target-dir/usr/lib
	echo "NAME=\"Clear Linux Software for Intel Architecture\"" | sudo tee "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "VERSION=1" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "ID=clear-linux-os" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "VERSION_ID=$version" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "PRETTY_NAME=\"Clear Linux Software for Intel Architecture\"" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "ANSI_COLOR=\"1;35\"" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "HOME_URL=\"https://clearlinux.org\"" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "SUPPORT_URL=\"https://clearlinux.org\"" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	echo "BUG_REPORT_URL=\"https://bugs.clearlinux.org/jira\"" | sudo tee -a "$env_name"/target-dir/usr/lib/os-release > /dev/null
	sudo mkdir -p "$env_name"/target-dir/usr/share/clear/bundles

	# state files & dirs
	sudo mkdir -p "$env_name"/state
	# TODO(castulo): add more dirs & files as neeeded by the tests

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
	local deletable=true
	validate_path "$env_name"
	# since the action to be performed is very destructive, at least
	# make sure the directory does look like a test environment
	for var in "state" "target-dir" "web-dir"; do
		if [ ! -d "$env_name/$var" ]; then
			deletable=false
		fi
	done
	if [ "$deletable" = true ]; then
		sudo rm -rf "$env_name"
	else
		echo "The name provided doesn't seem to be a valid test environment"
		return 1
	fi

}

# Creates a bundle in the test environment. The bundle can contain files, directories or symlinks.
create_bundle() { 

	cb_usage() { 
		echo "Usage:"
		echo -e "\\tcreate_bundle [-L] [-n] <bundle_name> [-v] <version> [-d] <list of dirs> [-f] <list of files> [-l] <list of links> ENV_NAME"
		echo "Options:"
		echo -e "\\t-L\\tWhen the flag is selected the bundle will be 'installed' in the target-dir, otherwise it will only be created in web-dir"
		echo -e "\\t-n\\tThe name of the bundle to be created, if not specified a name will be autogenerated"
		echo -e "\\t-v\\tThe version for the bundle, if non selected version 10 will be used"
		echo -e "\\t-d\\tComma-separated list of directories to be included in the bundle"
		echo -e "\\t-f\\tComma-separated list of files to be created and included in the bundle"
		echo -e "\\t-l\\tComma-separated list of symlinks to be created and included in the bundle,"
		echo "Notes:"
		echo -e "\\t- if no option is selected, a minimal bundle will be created with only one directory"
		echo -e "\\t- for every symlink created a related file will be created and added to the bundle as well"
		echo -e "\\t- if the '-f' or '-l' options are used, and the directories where the files live don't exist,"
		echo -e "\\t  they will be automatically created and added to the bundle for each file"
		echo -e "\\nExample of usage:\\n"
		echo -e "\\tThe following command will create a bundle named 'test-bundle', which will include three directories,"
		echo -e "\\tfour files, and one symlink (they will be added to the bundle's manifest), all these resources will also"
		echo -e "\\tbe tarred. The bundle's manifest will be added to the MoM."
		echo -e "\\n\\tcreate_bundle -n test-bundle -f /usr/bin/test-1,/usr/bin/test-2,/etc/systemd/test-3 -l /etc/test-link my_test_env"
		echo -e "\\n"
		
	}
	
	local OPTIND
	local opt
	local dir_list
	local file_list
	local link_list
	local version
	local bundle_name
	local env_name
	local files_path
	local version_path
	local manifest
	local local_bundle=false

	set -f  # turn off globbing
	while getopts :v:d:f:l:n:L opt; do
		case "$opt" in
			d)	IFS=,
				read -r -a dir_list <<< "$OPTARG" ;;
			f)	IFS=,
				read -r -a file_list <<< "$OPTARG" ;;
			l)	IFS=,
				read -r -a link_list <<< "$OPTARG" ;;
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
	if [ -z "$dir_list" ] && [ -z "$file_list" ] && [ -z "$link_list" ] ; then
		# if nothing was specified to be created, at least create
		# one directory which is the bare minimum for a bundle
		dir_list=(/usr/bin)
	fi
	# all bundles should include their own tracking file, so append it to the
	# list of files to be created in the bundle
	file_list+=(/usr/share/clear/bundles/"$bundle_name")
	
	# get useful paths
	if [ "$(validate_path "$env_name")" ]; then
		echo "Please specify a valid environment"
		return 1
	fi
	version_path="$env_name"/web-dir/"$version"
	files_path="$version_path"/files
	target_path="$env_name"/target-dir

	# 1) create the initial manifest
	manifest=$(create_manifest "$version_path" "$bundle_name")
	echo "Manifest -> $manifest"  # TODO(castulo): remove this msg when finish development
	
	# 2) Create one directory for the bundle and add it the requested
	# times to the manifest.
	# Every bundle has to have at least one directory,
	# hashes in directories vary depending on owner and permissions,
	# so one directory hash can be reused many times
	bundle_dir=$(create_dir "$files_path")
	echo "Directory -> $bundle_dir"  # TODO(castulo): remove this msg when finish development
	# Create a zero pack for the bundle and add the directory to it
	sudo tar -cf "$version_path"/pack-"$bundle_name"-from-0.tar --exclude="$bundle_dir"/*  "$bundle_dir"
	for val in "${dir_list[@]}"; do
		add_to_manifest "$manifest" "$bundle_dir" "$val"
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$val"
		fi
	done
	
	# 3) Create the requested file(s)
	for val in "${file_list[@]}"; do
		# if the directories the file is don't exist, add them to the
		# bundle (except for the dir of the tracking file)
		fdir=$(dirname "$val")
		if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ] && [ "$fdir" != "/usr/share/clear/bundles" ]; then
			bundle_dir=$(create_dir "$files_path")
			add_to_manifest "$manifest" "$bundle_dir" "$fdir"
			# add each one of the directories of the path if they are not in the manifest already
			while [ $(dirname "$fdir") != "/" ]; do
				fdir=$(dirname "$fdir")
				if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ]; then
					add_to_manifest "$manifest" "$bundle_dir" "$fdir"
				fi
			done
		fi
		bundle_file=$(create_file "$files_path")
		echo "File -> $bundle_file"  # TODO(castulo): remove this msg when finish development
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
		# add it to the bundle
		fdir=$(dirname "$val")
		if [ ! "$(sudo cat "$manifest" | grep -x "D\\.\\.\\..*$fdir")" ]; then
			bundle_dir=$(create_dir "$files_path")
			add_to_manifest "$manifest" "$bundle_dir" "$fdir"
		fi
		bundle_link=$(create_link "$files_path")
		echo "Symlink -> $bundle_link"  # TODO(castulo): remove this msg when finish development
		add_to_manifest "$manifest" "$bundle_link" "$val"
		# Add the file pointed by the link to the zero pack of the bundle
		pfile=$(readlink -f "$bundle_link")
		sudo tar -rf "$version_path"/pack-"$bundle_name"-from-0.tar "$pfile"
		if [ "$local_bundle" = true ]; then
			sudo mkdir -p "$target_path$(dirname "$val")"
			# if local_bundle is enabled copy the link to target-dir but also
			# copy the file it points to
			pfile_path=$(sudo cat "$manifest" | grep "$(basename "$pfile")" | awk '{ print $4 }')
			sudo cp "$pfile" "$target_path$pfile_path"
			sudo cp --preserve=links "$bundle_link" "$target_path$val"
		fi
	done
	
	# 5) Add the bundle to the MoM
	add_to_manifest "$version_path"/Manifest.MoM "$manifest" "$bundle_name"

	# 6) Create/renew manifest tars
	create_tar "$manifest"
	sudo rm -f "$version_path"/Manifest.MoM.tar
	create_tar "$version_path"/Manifest.MoM

	# 7) Create the subscription to the bundle if the local_bundle flag iss enabled
	if [ "$local_bundle" = true ]; then
		sudo touch "$target_path"/usr/share/clear/bundles/"$bundle_name"
	fi

}

# Creates a new test case based on a template
# Parameters:
# - NAME: the name (and path) of the test to be generated
generate_test() {

	local name=$1
	local path
	validate_param "$name"

	path=$(dirname "$name")/
	name=$(basename "$name")

	echo -e "#!/usr/bin/env bats\\n" > "$path$name".bats
	echo -e "load \"../testlib\"\\n" >> "$path$name".bats
	echo -e "setup() {\\n" >> "$path$name".bats
	echo -e "\\tcreate_test_environment \"\$TEST_NAME\"" >> "$path$name".bats
	echo -e "\\t# create_bundle -n <bundle_name> -f <file_1>,<file_2>,<file_N> \"\$TEST_NAME\"" >> "$path$name".bats
	echo -e "\\n}\\n" >> "$path$name".bats
	echo -e "teardown() {\\n" >> "$path$name".bats
	echo -e "\\tdestroy_test_environment \"\$TEST_NAME\"" >> "$path$name".bats
	echo -e "\\n}\\n" >> "$path$name".bats
	echo -e "@test \"<test description>\" {\\n" >> "$path$name".bats
	echo -e "\\trun sudo sh -c \"\$SWUPD <swupd_command> \$SWUPD_OPTS <command_options>\"" >> "$path$name".bats
	echo -e "\\t# <validations>" >> "$path$name".bats
	echo -e "\\n}" >> "$path$name".bats

}
