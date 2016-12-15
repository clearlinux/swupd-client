#!/usr/bin/env bats

load "../../swupdlib"

deltafile="10-100-0832c0c7fb9d05ac3088a493f430e81044a0fc03f81cba9eb89d3b7816ef3962-9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  chown_root "$DIR/web-dir/100/delta/$deltafile"
  cp "$DIR/web-dir/10/staged/0832c0c7fb9d05ac3088a493f430e81044a0fc03f81cba9eb89d3b7816ef3962" "$DIR/target-dir/testfile"
  chown_root "$DIR/target-dir/testfile"
  mkdir -p "$DIR/web-dir/100/staged"
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/pack-os-core-from-10.tar" delta/$deltafile staged/
}

teardown() {
  clean_tars 10
  clean_tars 100
  rmdir "$DIR/web-dir/100/staged"
  revert_chown_root "$DIR/web-dir/100/delta/$deltafile"
  sudo rm "$DIR/target-dir/testfile"
}

@test "update use delta pack" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  check_lines "$output"

  run sudo sh -c "$SWUPD hashdump $DIR/target-dir/testfile"

  echo "$output"
  [ "${lines[1]}" = "9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
