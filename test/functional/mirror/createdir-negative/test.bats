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

@test "mirror /etc/swupd is a file" {
  sudo rm -rf "$DIR/target-dir/etc/swupd"
  sudo touch "$DIR/target-dir/etc/swupd"
  [[ -f "$DIR/target-dir/etc/swupd" ]] && ! [[ -L "$DIR/target-dir/etc/swupd" ]]
  run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
  check_lines "$output"
}

@test "mirror /etc/swupd is a symlink to a file" {
  sudo rm -rf "$DIR/target-dir/etc/swupd"
  sudo rm -rf "$DIR/target-dir/foo"
  sudo touch "$DIR/target-dir/foo"
  sudo ln -s "$DIR/target-dir/foo" "$DIR/target-dir/etc/swupd"
  run sudo sh -c "$SWUPD mirror -s http://example.com/swupd-file $SWUPD_OPTS_MIRROR"
  check_lines "$output"
  sudo rm "$DIR/target-dir/foo"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
