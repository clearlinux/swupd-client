#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	set_current_version "$TEST_NAME" 33000

	# #################################################
	# statedir structure 4.2.1 (with grouped manifests)
	# #################################################
	# <statedir>
	# ├── manifest
	# │   ├── 32900
	# │   │   ├── Manifest.editors
	# │   │   └── Manifest.editors.8f7aca1643c1cd8b109774b65c83c0dad5a2d259348343a9395fd63f71640c63
	# │   ├── 33000
	# │   │   ├── Manifest.MoM
	# │   │   ├── Manifest.MoM.sig
	# │   │   ├── Manifest.emacs
	# │   │   └── Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d
	# │   └── Manifest-emacs-delta-from-32900-to-33000
	# ├── delta
	# ├── download
	# ├── pack-vim-from-32800-to-32900.tar
	# ├── staged
	# │   ├── 00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df
	# │   └── fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d
	# │

	# 3rd-party has both, data and cache

    # │
	# ├── 3rd-party
	# │

	# From here on it is just data

	# │
	# ├── bundles
	# │   ├── editors
	# │   ├── emacs
	# │   ├── os-core
	# │   └── vim
	# ├── telemetry
	# │   ├── 2.update.1.Dpjz7G
	# │   └── 2.verify.1.4hR0jx
	# ├── swupd_lock
	# └── version

	sudo mkdir -p "$ABS_STATE_DIR"/manifest/{32900,33000}
	sudo mkdir -p "$ABS_STATE_DIR"/{staged,delta,download}

	MOM="$ABS_STATE_DIR"/manifest/33000/Manifest.MoM ; sudo touch "$MOM"
	MOM_SIG="$ABS_STATE_DIR"/manifest/33000/Manifest.MoM.sig ; sudo touch "$MOM_SIG"
	FILE1="$ABS_STATE_DIR"/manifest/32900/Manifest.editors ; sudo touch "$FILE1"
	FILE2="$ABS_STATE_DIR"/manifest/32900/Manifest.editors.8f7aca1643c1cd8b109774b65c83c0dad5a2d259348343a9395fd63f71640c63 ; sudo touch "$FILE2"
	FILE3="$ABS_STATE_DIR"/manifest/33000/Manifest.emacs ; sudo touch "$FILE3"
	FILE4="$ABS_STATE_DIR"/manifest/33000/Manifest.emacs.7414710480607fa89c307ad11d145d8bb0cbdbdf3c0692f940d906b670f6fd5d ; sudo touch "$FILE4"
	FILE5="$ABS_STATE_DIR"/manifest/Manifest-emacs-delta-from-32900-to-33000 ; sudo touch "$FILE5"
	FILE6="$ABS_STATE_DIR"/pack-vim-from-32800-to-32900.tar ; sudo touch "$FILE6"
	FILE7="$ABS_STATE_DIR"/staged/00004ead4b4eb1cef1ebbe13849f6c47c21d325182f98cf8f5333e9a76d2e2df ; sudo touch "$FILE7"
	FILE8="$ABS_STATE_DIR"/staged/fffe891980af628e498b16a364b5b4d00377485370ccc4d09ac0fc072e85b31d ; sudo touch "$FILE8"

	sudo chmod -R 0700 "$ABS_STATE_DIR"

}

@test "CLN006: Clean cached files in the 4.2.1 statedir structure" {

	# during release 4.2.1 swupd used a statedir that contained both, cache
	# and data, but all manifest related artifcts were grouped within the
	# "manifest" directory. We need to make sure we are still cleaning those
	# files regardless of if the user used the --all flag or not since that
	# cache is not going to be used anymore anyway.

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		20 files removed
		.* KB freed
	EOM
	)
	assert_regex_is_output "$expected_output"

	# these old directories should be removed since are no longer used
	assert_dir_not_exists "$ABS_STATE_DIR"/delta
	assert_dir_not_exists "$ABS_STATE_DIR"/download
	assert_dir_not_exists "$ABS_STATE_DIR"/staged
	assert_dir_not_exists "$ABS_STATE_DIR"/manifest

	# files in the old structure should no longer be there either
	assert_file_not_exists "$ABS_STATE_DIR"/Manifest-emacs-delta-from-32900-to-33000
	assert_file_not_exists "$ABS_STATE_DIR"/pack-vim-from-32800-to-32900.tar

}

@test "CLN007: Dry run includes files in the 4.2.1 statedir structure" {

	# during release 4.2.1 swupd used a statedir that contained both, cache
	# and data, but all manifest related artifcts were grouped within the
	# "manifest" directory. If the user runs a "clean --dry-run" we should
	# include those files in the list of things that will be removed.

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --dry-run"

	assert_status_is "$SWUPD_OK"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/manifest/Manifest-emacs-delta-from-32900-to-33000
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/manifest/32900/Manifest\\..*
		$ABS_STATE_DIR/manifest/32900/Manifest\\..*
		$ABS_STATE_DIR/manifest/32900
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		$ABS_STATE_DIR/manifest/33000/Manifest\\..*
		$ABS_STATE_DIR/manifest/33000/Manifest\\..*
		$ABS_STATE_DIR/manifest/33000/Manifest\\..*
		$ABS_STATE_DIR/manifest/33000/Manifest\\..*
		$ABS_STATE_DIR/manifest/33000
		$ABS_STATE_DIR/manifest
	EOM
	)
	assert_regex_in_output "$expected_output"

	expected_output=$(cat <<-EOM
		Would remove 20 files
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
