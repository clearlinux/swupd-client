#!/usr/bin/env bats

load "../../swupdlib"

@test "check-update with no target version file available" {
  run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Querying server version." ]
  [ "${lines[3]}" = "Attempting to download version string to memory" ]
  [ "${lines[4]}" = "Unable to determine current OS version" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
