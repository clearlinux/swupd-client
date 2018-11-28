#!/usr/bin/env bats

@test "INS001: Check if all files were installed successfully in a full install" {
	# Binaries
	[ -x /usr/bin/swupd ]
	[ -x /usr/bin/verifytime ]

	# Manuals
	[ -s /usr/share/man/man4/update-triggers.target.4 ]
	[ -s /usr/share/man/man4/check-update.timer.4 ]
	[ -s /usr/share/man/man4/check-update.service.4 ]
	[ -s /usr/share/man/man4/swupd-update.timer.4 ]
	[ -s /usr/share/man/man4/swupd-update.service.4 ]
	[ -s /usr/share/man/man1/swupd.1 ]

	# System d files
	[ -s /usr/lib/systemd/system/swupd-update.timer ]
	[ -s /usr/lib/systemd/system/swupd-update.service ]
	[ -s /usr/lib/systemd/system/check-update.service ]
	[ -s /usr/lib/systemd/system/check-update.timer ]
	[ -s /usr/lib/systemd/system/verifytime.service ]

}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
