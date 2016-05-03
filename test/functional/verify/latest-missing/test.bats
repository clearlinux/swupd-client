#!/usr/bin/env bats

load "../../swupdlib"

@test "verify install using latest with missing version file on server" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m latest"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Unable to get latest version for install" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
