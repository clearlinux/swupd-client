#!/usr/bin/env bats

load "../../swupdlib"

f1=e324302d81e79a935137bd4ed6ab6c6792aa6d4356c9c227342eaa0db015c152
f2=826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550

setup() {
  clean_test_dir

  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_manifest_tar 10 test-bundle3

  create_fullfile_tar 10 $f1

  create_manifest_tar 20 MoM
  create_manifest_tar 20 os-core

  create_manifest_tar 30 MoM
  create_manifest_tar 30 os-core
  create_manifest_tar 30 test-bundle2
  create_manifest_tar 30 test-bundle3

  create_fullfile_tar 30 $f1
  create_fullfile_tar 30 $f2
}

teardown() {
  clean_tars 10
  clean_tars 20
  clean_tars 30
  clean_tars 10 files
  clean_tars 30 files
  revert_chown_root "$DIR/web-dir/10/files/$f1"
  revert_chown_root "$DIR/web-dir/30/files/$f1"
  revert_chown_root "$DIR/web-dir/30/files/$f2"
  sudo rm "$DIR/target-dir/os-core"
  sudo rm "$DIR/target-dir/tracked-file"
}

@test "update include a bundle from an older release" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 20 to 30" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  ignore_sigverify_error 8
  [ "${lines[8]}" = "Querying server manifest." ]
  ignore_sigverify_error 9
  [ "${lines[9]}" = "Downloading os-core pack for version 30" ]
  [ "${lines[10]}" = "Downloading test-bundle2 pack for version 30" ]
  [ "${lines[11]}" = "Downloading test-bundle3 pack for version 30" ]
  [ "${lines[12]}" = "Statistics for going from version 20 to version 30:" ]
  [ "${lines[13]}" = "    changed bundles   : 3" ]
  [ "${lines[14]}" = "    new bundles       : 0" ]
  [ "${lines[15]}" = "    deleted bundles   : 0" ]
  [ "${lines[16]}" = "    changed files     : 1" ]
  [ "${lines[17]}" = "    new files         : 1" ]
  [ "${lines[18]}" = "    deleted files     : 0" ]
  [ "${lines[19]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[20]}" = "Finishing download of update content..." ]
  [ "${lines[21]}" = "Staging file content" ]
  [ "${lines[22]}" = "Update was applied." ]
  [ "${lines[26]}" = "Update successful. System updated from version 20 to version 30" ]

  # changed file
  [ -f "$DIR/target-dir/os-core" ]

  # new file
  [ -f "$DIR/target-dir/tracked-file" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
