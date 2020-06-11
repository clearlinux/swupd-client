#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	set_current_version "$TEST_NAME" 33000

	# ######################################################################
	# cache/data structure post swupd 4.2.1 (split cache/data) for 3rd-party
	# ######################################################################
	# <cachedir>
	# └── 3rd-party
	#	  └── cache
	#	      ├── repo1
	#	      │   ├── delta
	#	      │   ├── download
	#	      │   ├── manifest
	#	      │   │   ├── 10
	#	      │   │   │   ├── Manifest.p11-kit
	#	      │   │   │   └── Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657
	#	      │   │   ├── 20
	#	      │   │   │   ├── Manifest.MoM
	#	      │   │   │   ├── Manifest.MoM.sig
	#	      │   │   │   ├── Manifest.emacs
	#	      │   │   │   └── Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d
	#	      │   │   ├── Manifest-emacs-delta-from-10-to-20
	#	      │   │   └── Manifest-vim-delta-from-10-to-20
	#	      │   ├── pack-blender-from-0-to-20.tar
	# 	      │   ├── pack-vim-from-10-to-20.tar
	# 	      │   ├── staged
	# 	      │   │   ├── 00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df
	# 	 	  │   │   └── fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d
	#         │   └── temp
	# 	      └── repo2
	#  	          ├── delta
	#             ├── download
	#	          ├── manifest
	#	          │   ├── 50
	#	          │   │   ├── Manifest.MoM
	#	          │   │   └── Manifest.MoM.sig
	#	          │   └── Manifest-qt-basic-delta-from-40-to-50
	#	          ├── pack-lib-imageformat-from-30-to-50.tar
	#	          ├── staged
	#	  	      │   └── ffaf66dab58a2074e5fe9dc6ab7bf8773c069817dcccf631148b6afc87166b4b
	#             └── temp

	# <datadir>
	# └── 3rd-party
	#	  ├── bundles
	#	  │   ├── editors
	#	  │   ├── os-core
	#	  │   └── vim
	#	  ├── swupd_lock
	#	  └── version

	# create the cache from "repo1"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	set_current_version "$TEST_NAME" 20 repo1

	sudo mkdir -p "$ABS_TP_MANIFEST_DIR"/{10,20}
	MOM=$(create_manifest "$ABS_TP_MANIFEST_DIR"/20 MoM 1 10)
	write_to_protected_file -a "$MOM" "M...	dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657	10	p11-kit\n"
	write_to_protected_file -a "$MOM" "M...	7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d	20	emacs\n"
	MOM_SIG="$MOM".sig ; sudo touch "$MOM_SIG"
	FILE1="$ABS_TP_MANIFEST_DIR"/10/Manifest.p11-kit ; sudo touch "$FILE1"
	FILE2="$ABS_TP_MANIFEST_DIR"/10/Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657 ; sudo touch "$FILE2"
	FILE3="$ABS_TP_MANIFEST_DIR"/20/Manifest.emacs ; sudo touch "$FILE3"
	FILE4="$ABS_TP_MANIFEST_DIR"/20/Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d ; sudo touch "$FILE4"
	FILE5="$ABS_TP_MANIFEST_DIR"/Manifest-emacs-delta-from-10-to-20 ; sudo touch "$FILE5"
	FILE6="$ABS_TP_MANIFEST_DIR"/Manifest-vim-delta-from-10-to-20 ; sudo touch "$FILE6"
	FILE7="$ABS_TP_CACHE_DIR"/pack-blender-from-0-to-20.tar ; sudo touch "$FILE7"
	FILE8="$ABS_TP_CACHE_DIR"/pack-vim-from-10-to-20.tar ; sudo touch "$FILE8"
	FILE9="$ABS_TP_STAGED_DIR"/00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df ; sudo touch "$FILE9"
	FILE10="$ABS_TP_STAGED_DIR"/fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d ; sudo touch "$FILE10"

	# create the cache from "repo2"
	create_third_party_repo -a "$TEST_NAME" 10 staging repo2
	set_current_version "$TEST_NAME" 50 repo2

	sudo mkdir -p "$ABS_TP_MANIFEST_DIR"/50
	MOM2=$(create_manifest "$ABS_TP_MANIFEST_DIR"/50 MoM 1 40)
	write_to_protected_file -a "$MOM2" "M...	dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead12345	10	blender\n"
	write_to_protected_file -a "$MOM2" "M...	1234510480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d	20	ansible\n"
	MOM2_SIG="$MOM2".sig ; sudo touch "$MOM2_SIG"
	FILE11="$ABS_TP_MANIFEST_DIR"/Manifest-qt-basic-delta-from-40-to-50 ; sudo touch "$FILE11"
	FILE12="$ABS_TP_CACHE_DIR"/pack-lib-imageformat-from-30-to-50.tar ; sudo touch "$FILE12"
	FILE13="$ABS_TP_STAGED_DIR"/ffaf66dab58a2074e5fe9dc6ab7bf8773c069817dcccf631148b6afc87166b4b ; sudo touch "$FILE13"

}

@test "TPR098: Clean 3rd-party cached files except manifests" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		12 files removed
		.* KB freed
		_____________________________
		 3rd-Party Repository: repo2
		_____________________________
		7 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# relevant manifests should still exist
	assert_file_exists "$FILE1"
	assert_file_exists "$FILE3"
	assert_file_exists "$MOM"
	assert_file_exists "$MOM_SIG"

	# the rest of the files should not exist
	assert_file_not_exists "$FILE2"
	assert_file_not_exists "$FILE4"
	assert_file_not_exists "$FILE5"
	assert_file_not_exists "$FILE6"
	assert_file_not_exists "$FILE7"
	assert_file_not_exists "$FILE8"
	assert_file_not_exists "$FILE9"
	assert_file_not_exists "$FILE10"

}

@test "TPR099: Clean all 3rd-party cached files including manifests" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --all"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		21 files removed
		.* KB freed
		_____________________________
		 3rd-Party Repository: repo2
		_____________________________
		13 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# everything should have been cleaned
	assert_file_not_exists "$FILE1"
	assert_file_not_exists "$FILE2"
	assert_file_not_exists "$FILE3"
	assert_file_not_exists "$FILE4"
	assert_file_not_exists "$FILE5"
	assert_file_not_exists "$FILE6"
	assert_file_not_exists "$FILE7"
	assert_file_not_exists "$FILE8"
	assert_file_not_exists "$FILE9"
	assert_file_not_exists "$FILE10"
	assert_file_not_exists "$FILE11"
	assert_file_not_exists "$FILE12"
	assert_file_not_exists "$FILE13"
	assert_file_not_exists "$MOM"
	assert_file_not_exists "$MOM_SIG"
	assert_file_not_exists "$MOM2"
	assert_file_not_exists "$MOM2_SIG"

}

@test "TPR100: Dry run clean to show the 3rd-party files that would be removed from the cache" {

	run sudo sh -c "$SWUPD 3rd-party clean $SWUPD_OPTS --dry-run"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/pack-.*-from-.*-to-20.tar
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/pack-.*-from-.*-to-20.tar
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/delta
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/download
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/staged/.*
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/staged/.*
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/staged
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/temp
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/manifest/Manifest-.*-delta-from-10-to-20
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/manifest/Manifest-.*-delta-from-10-to-20
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/manifest/.0/Manifest\\..*\\..*
		$ABS_STATE_DIR/3rd-party/repo1/cache/.*repo1/manifest/.0/Manifest\\..*\\..*
		Would remove 12 files
		Aproximatelly .* KB would be freed
		_____________________________
		 3rd-Party Repository: repo2
		_____________________________
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/pack-lib-imageformat-from-30-to-50.tar
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/delta
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/download
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/staged/ffaf66dab58a2074e5fe9dc6ab7bf8773c069817dcccf631148b6afc87166b4b
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/staged
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/temp
		$ABS_STATE_DIR/3rd-party/repo2/cache/.*repo2/manifest/Manifest-qt-basic-delta-from-40-to-50
		Would remove 7 files
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
	assert_file_exists "$MOM"
	assert_file_exists "$MOM_SIG"
	assert_file_exists "$MOM"
	assert_file_exists "$MOM_SIG"
	assert_file_exists "$MOM2"
	assert_file_exists "$MOM2_SIG"

}
