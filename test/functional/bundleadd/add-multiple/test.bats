#!/usr/bin/env bats

load "../../swupdlib"

t1_hash="6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e"
t2_hash="ea03fbc967f7e566d5e6aebc5c3494858d8fce9ae16dbd14031d43abbe714fc7"
t3_hash="0e583ff2aa785f042115b650b6fb193dac8c1eb373c0b8d2cf8ca29644d310c2"
t4_hash="e431ce75fd6cc6e5c8261a9dbbaf400f66c43aba871b75e19aa92e308861fee1"
t5_hash="512b7187c47b9c8166b61b160fc57297b0779ce4cdeae1eae057bacb23618357"
t6_hash="de3efc35f487ca0e1a486fb1b1bd35faa67608eb15953c788a0d47d983cece8b"
t7_hash="cf862a7e8d709b1d38dcbe260fc1f52c28ebb2284adb3934391589fb5d74e7ad"
t8_hash="fba2285c3d8e855e34019d19e3a4d302fc6ef56d12f00a091c5a2320948310e4"
t9_hash="2d64752982386688d47ab305a982a78609ea05f18589a07e7e927b6123e24853"
t10_hash="6a7b1e8538e384c02005bede2604e61126cb8cca40c3937335d30200dedef884"
t11_hash="0253825c1e86a2621cf7a9bf0dbc78f7ed94b673692d74229f173eafdf062a88"
t12_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_fullfile_tar 10 $t1_hash
  create_fullfile_tar 10 $t2_hash
  create_fullfile_tar 10 $t3_hash
  create_fullfile_tar 10 $t4_hash
  create_fullfile_tar 10 $t5_hash
  create_fullfile_tar 10 $t6_hash
  create_fullfile_tar 10 $t7_hash
  create_fullfile_tar 10 $t8_hash
  create_fullfile_tar 10 $t9_hash
  create_fullfile_tar 10 $t10_hash
  create_fullfile_tar 10 $t11_hash
  create_fullfile_tar 10 $t12_hash
  chown_root "$DIR/web-dir/10/staged"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle1-from-0.tar" --exclude=staged/$t1_hash/* --exclude=staged/$t12_hash staged/$t1_hash
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle2-from-0.tar" staged/$t12_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir"
  revert_chown_root "$DIR/web-dir/10/staged/$t1_hash"
  revert_chown_root "$DIR/web-dir/10/staged/$t2_hash"
  revert_chown_root "$DIR/web-dir/10/Manifest.os-core"
  revert_chown_root "$DIR/web-dir/10/Manifest.test-bundle1"
  revert_chown_root "$DIR/web-dir/10/Manifest.test-bundle2"
  sudo rm -rf "$DIR/target-dir/usr/bin/"
  sudo rm "$DIR/target-dir/usr/foo"
}

@test "bundle-add add multiple bundles" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"

  check_lines "$output"
  [ "$status" -eq 0 ]
  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
