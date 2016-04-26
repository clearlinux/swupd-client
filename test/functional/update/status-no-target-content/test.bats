#!/usr/bin/env bats

load "../../swupdlib"

@test "update --status with no target content" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

  echo "$output"
  [ "${lines[2]}" = "Querying server version." ]
  [ "${lines[3]}" = "Attempting to download version string to memory" ]
  [ "${lines[4]}" = "Cannot determine current OS version" ]
  [ "${lines[5]}" = "Latest server version: 100" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
