#!/usr/bin/env bats

load "../../swupdlib"

server_pid=""
port=""

setup() {
  for i in $(seq 8080 8180); do
    "$DIR/server.py" $i &
    sleep .5
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

  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
