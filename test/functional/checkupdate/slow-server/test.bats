#!/usr/bin/env bats

load "../../swupdlib"

server_pid=""
port=""

setup() {
  # extra debug for touchy test
  exec 21> $BATS_TEST_DIRNAME/debug.log
  BASH_XTRACEFD=21
  set -x

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
  # wait until server becomes available by expecting a successful curl
  for i in $(seq 1 10); do
    run curl http://localhost:$port/
    if [ "$status" -ne 0 ]; then
      sleep .5
      continue
    else
      break
    fi
  done

  run sudo sh -c "$SWUPD check-update $slow_opts"

  [ "$status" -eq 0 ]
  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
