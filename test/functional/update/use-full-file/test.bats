#!/usr/bin/env bats

load "../../swupdlib"

targetfile=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

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
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "update use full file" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  ignore_sigverify_error 8
  [ "${lines[8]}" = "Querying server manifest." ]
  ignore_sigverify_error 9
  [ "${lines[9]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[10]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[11]}" = "    changed bundles   : 1" ]
  [ "${lines[12]}" = "    new bundles       : 0" ]
  [ "${lines[13]}" = "    deleted bundles   : 0" ]
  [ "${lines[14]}" = "    changed files     : 1" ]
  [ "${lines[15]}" = "    new files         : 0" ]
  [ "${lines[16]}" = "    deleted files     : 0" ]
  [ "${lines[17]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[18]}" = "Finishing download of update content..." ]
  [ "${lines[19]}" = "Staging file content" ]
  [ "${lines[20]}" = "Update was applied." ]
  [ "${lines[24]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
