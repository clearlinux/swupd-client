#!/usr/bin/env bats

load "../../swupdlib"

deltafile="10-100-9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
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

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  [ "${lines[8]}" = "Querying server manifest." ]
  [ "${lines[9]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[10]}" = "Extracting pack." ]
  [ "${lines[11]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[12]}" = "    changed manifests : 1" ]
  [ "${lines[13]}" = "    new manifests     : 0" ]
  [ "${lines[14]}" = "    deleted manifests : 0" ]
  [ "${lines[15]}" = "    changed files     : 1" ]
  [ "${lines[16]}" = "    new files         : 0" ]
  [ "${lines[17]}" = "    deleted files     : 0" ]
  [ "${lines[18]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[19]}" = "Finishing download of update content..." ]
  [ "${lines[20]}" = "Staging file content" ]
  [ "${lines[21]}" = "Update was applied." ]
  [ "${lines[25]}" = "Update successful. System updated from version 10 to version 100" ]

  run sudo sh -c "$SWUPD hashdump $DIR/target-dir/testfile"

  echo "$output"
  [ "${lines[2]}" = "9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
