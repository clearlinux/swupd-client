#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  sudo mkdir -p "$DIR/target-dir/usr/share/defaults/swupd/"
  echo 4 | sudo tee "$DIR/target-dir/usr/share/defaults/swupd/format"
  sudo mkdir -p "$DIR/target-dir/etc"
}

teardown() {
  sudo rm -rf "$DIR/target-dir/etc/swupd"
  sudo rm -rf "$DIR/target-dir/foo"
}

@test "mirror /etc/swupd does not exist" {
  sudo rm -rf "$DIR/target-dir/etc/swupd"
  run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
  check_lines "$output"
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_contenturl) ]]
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_versionurl) ]]
}

@test "mirror /etc/swupd already exists" {
  sudo mkdir "$DIR/target-dir/etc/swupd"
  run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
  check_lines "$output"
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_contenturl) ]]
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_versionurl) ]]
}

@test "mirror /etc/swupd is a symlink to a directory" {
  sudo rm -rf "$DIR/target-dir/etc/swupd"
  sudo rm -rf "$DIR/target-dir/foo"
  sudo mkdir "$DIR/target-dir/foo"
  sudo ln -s "$DIR/target-dir/foo" "$DIR/target-dir/etc/swupd"
  run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
  check_lines "$output"
  ! [[ -L "$DIR/target-dir/etc/swupd" ]]
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_contenturl) ]]
  [[ "http://example.com/swupd-file" == $(<$DIR/target-dir/etc/swupd/mirror_versionurl) ]]
  sudo rm -rf "$DIR/target-dir/foo"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
