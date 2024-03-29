#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	set_current_version "$TEST_NAME" 33000

	# ###########################
	# statedir structure <= 4.2.0
	# ###########################
	# <statedir>
	# ├── 32900
	# │   ├── Manifest.p11-kit
	# │   └── Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657
	# ├── 33000
	# │   ├── Manifest.MoM
	# │   ├── Manifest.MoM.sig
	# │   ├── Manifest.emacs
	# │   └── Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d
	# ├── delta
	# ├── download
	# ├── Manifest-emacs-delta-from-32900-to-33000
	# ├── pack-vim-from-32800-to-32900.tar
	# ├── staged
	# │   ├── 00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df
	# │   └── fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d

	# 3rd-party has both, data and cache

	# │
	# ├── 3rd-party
	# │

	# From here on it is just data

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

	sudo mkdir -p "$ABS_STATE_DIR"/{32900,33000,delta,download,staged}

	MOM="$ABS_STATE_DIR"/33000/Manifest.MoM ; sudo touch "$MOM"
	MOM_SIG="$ABS_STATE_DIR"/33000/Manifest.MoM.sig ; sudo touch "$MOM_SIG"
	FILE1="$ABS_STATE_DIR"/32900/Manifest.p11-kit ; sudo touch "$FILE1"
	FILE2="$ABS_STATE_DIR"/32900/Manifest.p11-kit.dfd541b1ac9256982044848f2128f51e23ddbb72f68e9de43adaf8f7ead93657 ; sudo touch "$FILE2"
	FILE3="$ABS_STATE_DIR"/33000/Manifest.emacs ; sudo touch "$FILE3"
	FILE4="$ABS_STATE_DIR"/33000/Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d ; sudo touch "$FILE4"
	FILE5="$ABS_STATE_DIR"/Manifest-emacs-delta-from-32900-to-33000 ; sudo touch "$FILE5"
	FILE6="$ABS_STATE_DIR"/pack-vim-from-32800-to-32900.tar ; sudo touch "$FILE6"
	FILE7="$ABS_STATE_DIR"/staged/00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df ; sudo touch "$FILE7"
	FILE8="$ABS_STATE_DIR"/staged/fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d ; sudo touch "$FILE8"

	sudo chmod -R 0700 "$ABS_STATE_DIR"

}

@test "CLN004: Clean cached files in the pre 4.2.1 statedir structure" {

	# before release 4.2.0 swupd used a statedir that contained both, cache
	# and data. We need to make sure we are still cleaning that in case users
	# have the old statedir structure still in their system. We should clean
	# everything in the old structure regardless of if the user used the
	# --all flag or not since that cache is not going to be used anymore anyway.

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		19 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# these old directories should be removed since are no longer used
	assert_dir_not_exists "$ABS_STATE_DIR"/delta
	assert_dir_not_exists "$ABS_STATE_DIR"/download
	assert_dir_not_exists "$ABS_STATE_DIR"/staged
	assert_dir_not_exists "$ABS_STATE_DIR"/32900
	assert_dir_not_exists "$ABS_STATE_DIR"/33000

	# files in the old structure should no longer be there either
	assert_file_not_exists "$ABS_STATE_DIR"/Manifest-emacs-delta-from-32900-to-33000
	assert_file_not_exists "$ABS_STATE_DIR"/pack-vim-from-32800-to-32900.tar

}

@test "CLN005: Dry run includes files in the pre 4.2.1 statedir structure" {

	# before release 4.2.0 swupd used a statedir that contained both, cache
	# and data. If the user runs a "clean --dry-run" we should include those
	# files in the list of things that will be removed.

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --dry-run"

	assert_status_is "$SWUPD_OK"
	assert_in_output "$ABS_DELTA_DIR"
	assert_in_output "$ABS_DOWNLOAD_DIR"
	assert_in_output "$ABS_STAGED_DIR"
	assert_in_output "$ABS_TEMP_DIR"
	assert_in_output "$ABS_STATE_DIR/pack-vim-from-32800-to-32900.tar"
	assert_in_output "$ABS_STATE_DIR/Manifest-emacs-delta-from-32900-to-33000"
	assert_in_output "$ABS_STATE_DIR/staged"
	assert_in_output "$ABS_STATE_DIR/delta"
	assert_in_output "$ABS_STATE_DIR/download"
	assert_in_output "$ABS_STATE_DIR/32900"
	assert_in_output "$ABS_STATE_DIR/33000"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/staged/.*
		$ABS_STATE_DIR/staged/.*
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/32900/Manifest\\..*
		$ABS_STATE_DIR/32900/Manifest\\..*
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/33000/Manifest\\..*
		$ABS_STATE_DIR/33000/Manifest\\..*
		$ABS_STATE_DIR/33000/Manifest\\..*
		$ABS_STATE_DIR/33000/Manifest\\..*
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		Would remove 19 files
		Aproximatelly .* KB would be freed
	EOM
	)
	assert_regex_in_output "$expected_output"

	# non of the files should be removed since it was just a dry run
	assert_file_exists "$MOM"
	assert_file_exists "$MOM_SIG"
	assert_file_exists "$FILE1"
	assert_file_exists "$FILE2"
	assert_file_exists "$FILE3"
	assert_file_exists "$FILE4"
	assert_file_exists "$FILE5"
	assert_file_exists "$FILE6"
	assert_file_exists "$FILE7"
	assert_file_exists "$FILE8"

}
