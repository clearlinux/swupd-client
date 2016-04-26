#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  mkdir -p "$DIR/target-dir/usr/share/clear/bundles/"
  touch "$DIR/target-dir/usr/share/clear/bundles/test-bundle"
  touch "$DIR/target-dir/test-file"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
}

@test "bundle-remove remove bundle containing a file" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Deleting bundle files..." ]
  [ "${lines[4]}" = "Total deleted files: 1" ]
  [ "${lines[5]}" = "Untracking bundle from system..." ]
  [ "${lines[6]}" = "Success: Bundle removed" ]
  [ ! -f "$DIR/target-dir/test-file" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
