#!/usr/bin/env bats

load "../../swupdlib"

f1=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

setup() {
  clean_test_dir

  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 100 MoM
  create_manifest_tar 100 os-core
  create_manifest_tar 100 test-bundle1

  create_fullfile_tar 100 $f1
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$f1"
  sudo rmdir "$DIR/target-dir/os-core"
  sudo rmdir "$DIR/target-dir/usr/bin"
}

@test "update always add os-core to tracked bundles" {
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
  [ "${lines[10]}" = "Downloading test-bundle1 pack for version 100" ]
  [ "${lines[11]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[12]}" = "    changed bundles   : 2" ]
  [ "${lines[13]}" = "    new bundles       : 0" ]
  [ "${lines[14]}" = "    deleted bundles   : 0" ]
  [ "${lines[15]}" = "    changed files     : 2" ]
  [ "${lines[16]}" = "    new files         : 0" ]
  [ "${lines[17]}" = "    deleted files     : 0" ]
  [ "${lines[18]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[19]}" = "Finishing download of update content..." ]
  [ "${lines[20]}" = "Staging file content" ]
  [ "${lines[21]}" = "Update was applied." ]
  [ "${lines[25]}" = "Update successful. System updated from version 10 to version 100" ]

  # changed files (hashes didn't change, only the versions)
  [ -d "$DIR/target-dir/os-core" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
