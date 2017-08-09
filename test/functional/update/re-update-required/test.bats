#!/usr/bin/env bats

load "../../swupdlib"

targetfile=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e
targetfilecross=938b2569476c3cf665afa9ea5ac2e97bace1e098b8c7f913db0f00e6b96c612e
targetfile110=82dd2d70ab95d9500469b4ba0ee12dcda50aa566263a70b52518e5bc3ab6e3c4
targetfilefmt=512b7187c47b9c8166b61b160fc57297b0779ce4cdeae1eae057bacb23618357

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core

  create_manifest_tar 20 MoM
  sign_manifest_mom 20
  create_manifest_tar 20 os-core
  create_fullfile_tar 20 $targetfilecross
  create_fullfile_tar 20 $targetfile
  create_fullfile_tar 20 $targetfilefmt

  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  create_fullfile_tar 100 $targetfilecross
  create_fullfile_tar 100 $targetfile
  create_fullfile_tar 100 $targetfilefmt

  create_manifest_tar 110 MoM
  sign_manifest_mom 110
  create_manifest_tar 110 os-core
  create_fullfile_tar 110 $targetfile110
  create_fullfile_tar 110 $targetfile
  create_fullfile_tar 110 $targetfilefmt

  # reasonable gitignore rules prohibit "swupd"
  # because of how gitignore works, this rule can not be negated unless
  # swupd is always treated as a directory. Always just create the directory
  # at the beginning of the test and copy format into it.
  # https://stackoverflow.com/a/5534865
  sudo mkdir -p "$DIR/target-dir/usr/share/defaults/swupd/"
  sudo cp "$DIR/format" "$DIR/target-dir/usr/share/defaults/swupd/format"
  sudo cp "$DIR/os-release" "$DIR/target-dir/usr/lib/os-release"
}

teardown() {
  clean_tars 10
  clean_tars 20
  clean_tars 20 files
  clean_tars 100
  clean_tars 100 files
  clean_tars 110
  clean_tars 110 files
  revert_chown_root "$DIR/web-dir/20/files/$targetfile"
  revert_chown_root "$DIR/web-dir/20/files/$targetfilecross"
  revert_chown_root "$DIR/web-dir/20/files/$targetfilefmt"
  revert_chown_root "$DIR/web-dir/100/files/$targetfile"
  revert_chown_root "$DIR/web-dir/100/files/$targetfilecross"
  revert_chown_root "$DIR/web-dir/100/files/$targetfilefmt"
  revert_chown_root "$DIR/web-dir/110/files/$targetfile"
  revert_chown_root "$DIR/web-dir/110/files/$targetfile110"
  revert_chown_root "$DIR/web-dir/110/files/$targetfilefmt"
  sudo cp "$DIR/format" "$DIR/target-dir/usr/share/defaults/swupd/format"
  sudo cp "$DIR/os-release" "$DIR/target-dir/usr/lib/os-release"
  sudo rm -rf "$DIR/target-dir/usr/bin/"
}

@test "update re-update required" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT"

  [ "$status" -eq 0 ]
  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
