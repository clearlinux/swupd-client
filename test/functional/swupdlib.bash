# NOTE: source this file from a *.bats file

# The location of the swupd_* binaries
export SRCDIR="$BATS_TEST_DIRNAME/../../../../"

export SWUPD="$SRCDIR/swupd"

export DIR="$BATS_TEST_DIRNAME"

export STATE_DIR="$BATS_TEST_DIRNAME/state"

export SWUPD_OPTS="-S $STATE_DIR -p $DIR/target-dir -F staging -u file://$DIR/web-dir -C $BATS_TEST_DIRNAME/../../Swupd_Root.pem -I"

export SWUPD_OPTS_NO_FMT="-S $STATE_DIR -p $DIR/target-dir -u file://$DIR/web-dir -C $BATS_TEST_DIRNAME/../../Swupd_Root.pem -I"

export SWUPD_OPTS_NO_CERT="-S $STATE_DIR -p $DIR/target-dir -F staging -u file://$DIR/web-dir"

export CERT="$BATS_TEST_DIRNAME/Swupd_Root.pem"

export CERTCONF="$BATS_TEST_DIRNAME/certattributes.cnf"

# Maintain compatibility
clean_test_dir() {
  clean_state_dir
}

clean_state_dir() {
  sudo rm -rf "$STATE_DIR"
}

clean_target_dir() {
  sudo rm -rf "$DIR/target-dir"
}

clean_web_dir() {
  sudo rm -rf "$DIR/web_dir"
}


clean_tars() {
  local ver=$1
  local path=
  if [ -n $2 ]; then
    path="$DIR/web-dir/$ver/$2"
  else
    path="$DIR/web-dir/$ver"
  fi
  pushd $path
  sudo rm *.tar
  popd
}

chown_root() {
  sudo chown root:root "$@"
}

revert_chown_root() {
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$@"
}

create_manifest_tar() {
  local ver=$1
  local name=$2
  chown_root "$DIR/web-dir/$ver/Manifest.$name"
  sudo tar -C "$DIR/web-dir/$ver" -cf "$DIR/web-dir/$ver/Manifest.$name.tar" Manifest.$name
}

create_fullfile_tar() {
  local ver=$1
  local hash="$2"
  local extra_arg=
  local dir="$DIR/web-dir/$ver/files"
  local path="$dir/$hash"
  chown_root "$path"
  if [ -d "$path" ]; then
    extra_arg="--exclude=$hash/*"
  else
    extra_arg=""
  fi
  sudo tar -C "$dir" -cf "$path.tar" $extra_arg $hash
}

# TODO several tests create packs, so it would be nice to encapsulate those steps
# create_pack_tar() {
#   local from_ver=$1
#   local to_ver=$2
#   local bundle=$3
#   local contents="${@:4}"
#   return
# }

check_lines() {
  local outputstr="$1"
  local outputfile="$DIR/lines-output"
  local checked="$DIR/lines-checked"
  local ignored="$DIR/../../ignore-list"
  local prog="$DIR/../../matcher.awk"

  echo "$outputstr" > "$outputfile"

  run awk -f "$prog" "$ignored" "$checked" "$outputfile"

  if [ $status -eq 1 ]; then
    echo "$output"
    echo -e "\nChecked lines versus actual output (note that actual output may contain ignored lines):\n"
    diff -u "$checked" "$outputfile"
  fi
}

# This generates the private key and self signed certificate
generate_cert() {
  openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 \
  -keyout $BATS_TEST_DIRNAME/private.pem -out $CERT \
  -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=www.example.com/DN=MixerCert" \
  -config $CERTCONF
}

sign_manifest_mom() {
  sudo openssl smime -sign -binary -in $BATS_TEST_DIRNAME/web-dir/$1/Manifest.MoM \
    -signer $SRCDIR/test/functional/Swupd_Root.pem -inkey $SRCDIR/test/functional/private.pem \
    -outform DER -out $BATS_TEST_DIRNAME/web-dir/$1/Manifest.MoM.sig
}

set_os_release() {
  sudo mkdir -p $DIR/target-dir/usr/lib
  echo "VERSION_ID=$1" | sudo tee $DIR/target-dir/usr/lib/os-release
}

# Create a target tree from a manifest
unpack_target_from_manifest(){
  local REL=$1
  local MANIFEST=$2
  exec 9< $DIR/web-dir/$REL/Manifest.$MANIFEST
  # Skip header
  while read -u 9 line
  do  [ -z "$line" ] && break
  done
  # Process lines
  mkdir -p "$DIR/target-dir"
  while read -u 9 code hash ver path
  do
      case "$code" in
	  ("F"*) sudo cp "$DIR/web-dir/$REL/staged/$hash" "$DIR/target-dir/$path" ;;
	  ("D"*) sudo mkdir -p "$DIR/target-dir/$path" ;;
      esac
  done
  exec 9<&-
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
