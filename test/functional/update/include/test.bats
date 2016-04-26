#!/usr/bin/env bats

load "../../swupdlib"

f1=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
f2=d56e570097c2932ff0454cb45ef37964c6a0792e0224013f9e1d837cc22aa9fa
f3=95a84952523cf67d92b9f4f03057ca1bf6e1ff018104f7f7ad27d5d4fb38b9ad
f4=82206f761ee900b5227656f443f9ac4beb32c25573da6b59bf597e894f4eceac
f5=6e5658ceb83e69365ded0684df8f38bcc632f71780ae10ff19b87616a03d6337
f6=7726ea82e871eb80c9a72ef95e83ae00e2e8097e7d99ce43b8e47afb1113414b
f7=34c8113c07c33167de84e9b9c5557895c958aece3519669706a63a0dafee2481
f8=39e14cebdd7f0cab5089a9cecf97f5e77b226d00325e22a0a9081eb544e0b6f0

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
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f1.tar" $f1 --exclude=$f1/*
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
