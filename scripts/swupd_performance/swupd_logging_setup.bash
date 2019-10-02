#!/usr/bin/bash

set -e

DIRECTORY_PREFIX="/var/log/swupd"
STAGING_DIRECTORY="$DIRECTORY_PREFIX"/staging/
CONFIG_DIRECTORY="/etc/swupd_perform_update_config"

check_for_root() {
	if [ "$EUID" -ne 0 ]
	then echo "Please run as root or with sudo"
		exit
	fi
}



setup_system() {
	systemctl unmask --now swupd-update.service && echo "Unmasking swupd-update.service succeeded!"
	systemctl unmask --now swupd-update.timer && echo "Unmasking swupd-update.timer succeeded!"
	override_swupd_unit_file
	setup_staging_directory
	populate_config

	echo ""
	echo "Perform system daemon reload"
	systemctl daemon-reload && echo "systemctl daemon-reload was successful!"
	
	echo ""
	echo "Perform swupd-update.timer restart"
	systemctl restart swupd-update.timer && echo "swupd-update.service was successfully scheduled!"
}

restore_system() {
	user_input=
	echo -n "Are you sure you want to clean files related to swupd performance (y/n)?"
	read -r user_input
	if [ "$user_input" != "y" ]; then
		echo "Exiting program! No cleanup performed"
		exit 0
	fi

	rm -r /etc/systemd/system/swupd-update.service || true
	systemctl mask --now swupd-update.service && echo "Masking swupd-update.service succeeded!"
	echo ""
	systemctl mask --now swupd-update.timer && echo "Masking swupd-update.timer succeeded!"
	echo ""

	if [ -d "$CONFIG_DIRECTORY" ]; then
		rm -rf "$CONFIG_DIRECTORY" && echo "Config directory removed"
		echo ""
	fi

	if [ -d "$STAGING_DIRECTORY" ]; then
		rm -rf "$STAGING_DIRECTORY" && echo "Staging directory & all the swupd update output files removed"
		echo ""
	fi

	echo "System restored to original state"
	exit 0
}

setup_staging_directory () {
	echo "Setup staging directory"

	if [ -d "$STAGING_DIRECTORY" ]; then
		echo "Staging directory already exists"
	else
		mkdir -p "$STAGING_DIRECTORY" && echo "Staging directory: $STAGING_DIRECTORY successfully created!"
	fi
}

populate_config() {
	echo ""
	echo "Setup performance monitoring script config dir"

	if [ -d "$CONFIG_DIRECTORY" ]; then
		echo "Config directory already exists"
		echo "Re-populate it"
	else
		mkdir "$CONFIG_DIRECTORY" && echo "Config directory created!"
		echo "Populate it"
	fi

	cp -r swupd_update_wrapper.sh "$CONFIG_DIRECTORY"
	cp -r swupd_update_cleaner.sh "$CONFIG_DIRECTORY"

# Setting the staging environment file to read folder prefix from
cat <<EOF > "$CONFIG_DIRECTORY"/folder_prefix_env_file
OUTFOLDER=$STAGING_DIRECTORY
EOF
}

override_swupd_unit_file() {
# Setting the systemd for the swupd-update.service
cat <<EOF > /etc/systemd/system/swupd-update.service
[Unit]
Description=Update Software content

[Service]
Type=oneshot
ExecStart=/usr/bin/bash -c "$CONFIG_DIRECTORY/swupd_update_wrapper.sh \$OUTFOLDER"
EnvironmentFile=$CONFIG_DIRECTORY/folder_prefix_env_file
ExecStartPost=/usr/bin/bash -c "$CONFIG_DIRECTORY/swupd_update_cleaner.sh \$OUTFOLDER"
EOF
}

parse_args() {
	if [ "$1" = "restore" ]; then
		restore_system
	else
		setup_system
	fi
}

# main
check_for_root
parse_args "$1"
