#!/usr/bin/env bats

load "../../swupdlib"

server_pid=""
port=""

targetfile="6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e"

setup() {
  # Skip this test if not running in Travis CI, because test takes too long for
  # local development. To run this locally do: TRAVIS=true make check
  if [ -z "${TRAVIS}" ]; then
    skip "Skipping slow test for local development, test only runs in CI"
  fi

  for i in $(seq 8081 8181); do
    pushd "$DIR"
    "./server.py" $i &
    sleep .5
    server_pid=$!
    if [ -d /proc/$server_pid ]; then
      port=$i
      break
    fi
    popd
  done

  clean_test_dir

  #create partial tarball
  mkdir "$DIR/state/"
  chown_root "$DIR/state"
  sudo chmod 700  "$DIR/state"

  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core

  # create_pack_tar 10 100 os-core $targetfile
  chown_root "$DIR/web-dir/100/staged/$targetfile"
  tar -C "$DIR/web-dir/100" -cjf "$DIR/web-dir/100/pack-os-core-from-10.tar" --exclude=staged/$targetfile/* staged/$targetfile
}

teardown() {
  # teardown only if in travis CI
  if [ -n "${TRAVIS}" ]; then
    sudo rm -rf "$DIR/target-dir/usr/bin"
    kill $server_pid
  fi
}

@test "update --download with a slow server" {
  slow_opts="-u http://localhost:$port/"

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

  run sudo sh -c "$SWUPD update $slow_opts $SWUPD_OPTS"

  [ "$status" -eq 0 ]
  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
