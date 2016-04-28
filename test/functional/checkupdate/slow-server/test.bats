#!/usr/bin/env bats

load "../../swupdlib"

server_pid=""
port=""

setup() {
  for i in $(seq 8080 8180); do
    "$DIR/server.py" $i &
    sleep .2
    server_pid=$!
    if [ -d /proc/$server_pid ]; then
      port=$i
      break
    fi
  done
}

teardown() {
  kill $server_pid
}

@test "check-update with a slow server" {
  slow_opts="-p $DIR/target-dir -F staging -u http://localhost:$port/"
  run sudo sh -c "$SWUPD check-update $slow_opts"

  echo "$output"
  [ "${lines[2]}" = "Querying server version." ]
  [ "${lines[3]}" = "Attempting to download version string to memory" ]
  [ "${lines[4]}" = "There is a new OS version available: 99990" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
