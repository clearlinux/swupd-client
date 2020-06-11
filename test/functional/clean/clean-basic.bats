#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	set_current_version "$TEST_NAME" 33000

	# ########################################################
	# cache/data structure post swupd 4.2.1 (split cache/data)
	# ########################################################
	# <cachedir>
	# └── cache
	#     ├── https_cdn.download.clearlinux.org_update
	#     │   ├── delta
	#     │   ├── download
	#     │   ├── manifest
	#     │   │   ├── 32900
	#     │   │   │   ├── Manifest.p11-kit
	#     │   │   │   └── Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657
	#     │   │   ├── 33000
	#     │   │   │   ├── Manifest.MoM
	#     │   │   │   ├── Manifest.MoM.sig
	#     │   │   │   ├── Manifest.emacs
	#     │   │   │   └── Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d
	#     │   │   ├── Manifest-emacs-delta-from-32900-to-33000
	#     │   │   └── Manifest-vim-delta-from-32900-to-33000
	#     │   ├── pack-blender-from-0-to-33000.tar
	#     │   ├── pack-vim-from-32800-to-32900.tar
	#     │   ├── staged
	#     │   │   ├── 00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df
	#     │   │   └── fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d
	#     │   └── temp
	#     └── https_other.clearlinux.update_site
	#         ├── delta
	#         ├── download
	#         ├── manifest
	#         │   ├── 32900
	#         │   │   ├── Manifest.MoM
	#         │   │   └── Manifest.MoM.sig
	#         │   └── Manifest-qt-basic-delta-from-33010-to-33140
	#         ├── pack-lib-imageformat-from-33090-to-33170.tar
	#         ├── staged
	#         │   └── ffaf66dab58a2074e5fe9dc6ab7bf8773c069817dcccf631148b6afc87166b4b
	#         └── temp

	# <datadir>
	# │
	# ├── bundles
	# │   ├── editors
	# │   ├── os-core
	# │   └── vim
	# ├── telemetry
	# │   ├── 2.packmissing.1.Zh11gI
	# │   └── 2.verify.1.4hR0jx
	# ├── swupd_lock
	# └── version

	sudo mkdir -p "$ABS_CACHE_DIR"/{delta,download,manifest,staged}
	sudo chmod 0700 "$ABS_CACHE_DIR"/staged
	sudo mkdir -p "$ABS_MANIFEST_DIR"/{32900,33000}

	MOM=$(create_manifest "$ABS_MANIFEST_DIR"/33000 MoM 1 32900)
	write_to_protected_file -a "$MOM" "M...	dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657	32900	p11-kit\n"
	write_to_protected_file -a "$MOM" "M...	7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d	33000	emacs\n"
	MOM_SIG="$MOM".sig ; sudo touch "$MOM_SIG"

	FILE1="$ABS_MANIFEST_DIR"/32900/Manifest.p11-kit ; sudo touch "$FILE1"
	FILE2="$ABS_MANIFEST_DIR"/32900/Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657 ; sudo touch "$FILE2"
	FILE3="$ABS_MANIFEST_DIR"/33000/Manifest.emacs ; sudo touch "$FILE3"
	FILE4="$ABS_MANIFEST_DIR"/33000/Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d ; sudo touch "$FILE4"
	FILE5="$ABS_MANIFEST_DIR"/Manifest-emacs-delta-from-32900-to-33000 ; sudo touch "$FILE5"
	FILE6="$ABS_MANIFEST_DIR"/Manifest-vim-delta-from-32900-to-33000 ; sudo touch "$FILE6"
	FILE7="$ABS_CACHE_DIR"/pack-blender-from-0-to-33000.tar ; sudo touch "$FILE7"
	FILE8="$ABS_CACHE_DIR"/pack-vim-from-32800-to-32900.tar ; sudo touch "$FILE8"
	FILE9="$ABS_STAGED_DIR"/00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df ; sudo touch "$FILE9"
	FILE10="$ABS_STAGED_DIR"/fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d ; sudo touch "$FILE10"
	# second url cache
	sudo mkdir -p "$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/{delta,download,manifest,staged,temp}
	sudo mkdir -p "$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/manifest/32900
	FILE11="$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/pack-lib-imageformat-from-33090-to-33170.tar ; sudo touch "$FILE11"
	FILE12="$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/manifest/32900/Manifest.MoM ; sudo touch "$FILE12"
	FILE13="$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/manifest/32900/Manifest.MoM.sig ; sudo touch "$FILE13"
	FILE14="$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/manifest/Manifest-qt-basic-delta-from-33010-to-33140 ; sudo touch "$FILE14"
	FILE15="$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site/staged/ffaf66dab58a2074e5fe9dc6ab7bf8773c069817dcccf631148b6afc87166b4b ; sudo touch "$FILE15"

}

@test "CLN001: Clean cached files except manifests" {

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		24 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# relevant manifests should still exist
	assert_file_exists "$ABS_MANIFEST_DIR"/32900/Manifest.p11-kit
	assert_file_exists "$ABS_MANIFEST_DIR"/33000/Manifest.emacs
	assert_file_exists "$ABS_MANIFEST_DIR"/33000/Manifest.MoM
	assert_file_exists "$ABS_MANIFEST_DIR"/33000/Manifest.MoM.sig

	# the rest of the files should not exist
	assert_file_not_exists "$FILE2"
	assert_file_not_exists "$FILE4"
	assert_file_not_exists "$FILE5"
	assert_file_not_exists "$FILE6"
	assert_file_not_exists "$FILE7"
	assert_file_not_exists "$FILE8"
	assert_dir_not_exists "$ABS_STAGED_DIR"
	assert_dir_not_exists "$ABS_DOWNLOAD_DIR"
	assert_dir_not_exists "$ABS_DELTA_DIR"
	assert_dir_not_exists "$ABS_STATE_DIR"/cache/https_other.clearlinux.update_site

}

@test "CLN002: Clean all cached files including manifests" {

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --all"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		33 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# everything should have been cleaned
	assert_dir_not_exists "$ABS_STATE_DIR"/cache

}

@test "CLN003: Dry run clean to show the files that would be removed from the cache" {

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --dry-run"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/cache/https_other.clearlinux.update_site
		Would remove 24 files
		Aproximatelly .* KB would be freed
	EOM
	)
	assert_regex_in_output "$expected_output"

	# files should not be deleted, just printed
	assert_file_exists "$FILE1"
	assert_file_exists "$FILE2"
	assert_file_exists "$FILE3"
	assert_file_exists "$FILE4"
	assert_file_exists "$FILE5"
	assert_file_exists "$FILE6"
	assert_file_exists "$FILE7"
	assert_file_exists "$FILE8"
	assert_file_exists "$FILE9"
	assert_file_exists "$FILE10"
	assert_file_exists "$FILE11"
	assert_file_exists "$FILE12"
	assert_file_exists "$FILE13"
	assert_file_exists "$FILE14"
	assert_file_exists "$FILE15"
	assert_file_exists "$MOM"
	assert_file_exists "$MOM_SIG"

}
