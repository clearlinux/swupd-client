#!/usr/bin/env bats

load "../../swupdlib"

f1=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
f2=5f79d142c1859a3d82e9391a9a23a5f697e9346714412b67c78729c5a7b6ae1f
f3=a325dc221e0e03802b5a81abdf7318074230ca1f980985032048b5b6ce193850
f4=d1f70150c56e960ca18bd636131fae747487ac93836429ada2a46f85b88af28f
f5=4185aaf172a7f8b0cffb79fa379dc2227edb7097f24a61f7ce5b3881e38a66a2
f6=e2c7d7bb180e8b44ce0fb4e58f3b2f3852d7fbbb0fafbe04e41159d006501a88
f7=01fe90f246616cc07c216fa4fc1444881a5f2804650989996a44d223e0bfb105
f8=9a94d945b3841b70df7117fd02cffa36e4f1e5b430703b8fd26c086bf1059e72

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle1.tar" Manifest.test-bundle1 Manifest.test-bundle1.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle2.tar" Manifest.test-bundle2 Manifest.test-bundle2.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle1.tar" Manifest.test-bundle1 Manifest.test-bundle1.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle2.tar" Manifest.test-bundle2 Manifest.test-bundle2.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle3.tar" Manifest.test-bundle3 Manifest.test-bundle3.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle4.tar" Manifest.test-bundle4 Manifest.test-bundle4.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle5.tar" Manifest.test-bundle5 Manifest.test-bundle5.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle6.tar" Manifest.test-bundle6 Manifest.test-bundle6.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle7.tar" Manifest.test-bundle7 Manifest.test-bundle7.signed
  sudo chown root:root "$DIR/web-dir/100/files/$f1"
  sudo chown root:root "$DIR/web-dir/100/files/$f2"
  sudo chown root:root "$DIR/web-dir/100/files/$f3"
  sudo chown root:root "$DIR/web-dir/100/files/$f4"
  sudo chown root:root "$DIR/web-dir/100/files/$f5"
  sudo chown root:root "$DIR/web-dir/100/files/$f6"
  sudo chown root:root "$DIR/web-dir/100/files/$f7"
  sudo chown root:root "$DIR/web-dir/100/files/$f8"
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f1.tar" --exclude=$f1/* $f1
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f2.tar" $f2
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f3.tar" $f3
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f4.tar" $f4
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f5.tar" $f5
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f6.tar" $f6
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f7.tar" $f7
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f8.tar" $f8
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100/files"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f1"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f2"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f3"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f4"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f5"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f6"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f7"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f8"
  sudo rmdir "$DIR/target-dir/usr/bin"
  sudo rm "$DIR/target-dir/usr/core"
  sudo rm "$DIR/target-dir/usr/foo2"
  sudo rm "$DIR/target-dir/usr/foo3"
  sudo rm "$DIR/target-dir/usr/foo4"
  sudo rm "$DIR/target-dir/usr/foo5"
  sudo rm "$DIR/target-dir/usr/foo6"
  sudo rm "$DIR/target-dir/usr/foo7"
}

@test "update with includes" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  [ "${lines[8]}" = "Querying server manifest." ]
  [ "${lines[9]}" = "Downloading test-bundle3 pack for version 100" ]
  [ "${lines[10]}" = "Downloading test-bundle4 pack for version 100" ]
  [ "${lines[11]}" = "Downloading test-bundle5 pack for version 100" ]
  [ "${lines[12]}" = "Downloading test-bundle2 pack for version 100" ]
  [ "${lines[13]}" = "Downloading test-bundle7 pack for version 100" ]
  [ "${lines[14]}" = "Downloading test-bundle6 pack for version 100" ]
  [ "${lines[15]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[16]}" = "Downloading test-bundle1 pack for version 100" ]
  [ "${lines[17]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[18]}" = "    changed manifests : 3" ]
  [ "${lines[19]}" = "    new manifests     : 0" ]
  [ "${lines[20]}" = "    deleted manifests : 0" ]
  [ "${lines[21]}" = "    changed files     : 3" ]
# FIXME not detecting new files from includes
  [ "${lines[22]}" = "    new files         : 0" ]
  [ "${lines[23]}" = "    deleted files     : 0" ]
  [ "${lines[24]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[25]}" = "Finishing download of update content..." ]
  [ "${lines[26]}" = "Staging file content" ]
  [ "${lines[27]}" = "Update was applied." ]
  [ "${lines[31]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/core" ]
  [ -f "$DIR/target-dir/usr/foo2" ]
  [ -f "$DIR/target-dir/usr/foo3" ]
  [ -f "$DIR/target-dir/usr/foo4" ]
  [ -f "$DIR/target-dir/usr/foo5" ]
  [ -f "$DIR/target-dir/usr/foo6" ]
  [ -f "$DIR/target-dir/usr/foo7" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
