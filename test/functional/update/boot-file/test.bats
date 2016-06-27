#!/usr/bin/env bats

load "../../swupdlib"

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  create_manifest_tar 100 os-core
  create_fullfile_tar 100 $targetfile
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$targetfile"
  sudo rm "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "update add boot file" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[3]}" = "Attempting to download version string to memory" ]
  [ "${lines[4]}" = "Update started." ]
  [ "${lines[5]}" = "Querying server version." ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  [ "${lines[7]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[8]}" = "Querying current manifest." ]
  ignore_sigverify_error 9
  [ "${lines[9]}" = "Querying server manifest." ]
  ignore_sigverify_error 10
  [ "${lines[10]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[11]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[12]}" = "    changed manifests : 1" ]
  [ "${lines[13]}" = "    new manifests     : 0" ]
  [ "${lines[14]}" = "    deleted manifests : 0" ]
  [ "${lines[15]}" = "    changed files     : 0" ]
  [ "${lines[16]}" = "    new files         : 1" ]
  [ "${lines[17]}" = "    deleted files     : 0" ]
  [ "${lines[18]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[19]}" = "Finishing download of update content..." ]
  [ "${lines[20]}" = "Staging file content" ]
  [ "${lines[21]}" = "Update was applied." ]
  [ "${lines[24]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
