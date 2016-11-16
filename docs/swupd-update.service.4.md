swupd-update.service(4) -- System Unit
==================================

## SYNOPSIS

`swupd-update.service`
`/usr/lib/systemd/system/swupd-update.service`

## DESCRIPTION

Instructs the `systemd(1)` system daemon how to start and control the
`swupd` service to perform an update.

When this unit runs, the output will be sent to the systemd journal, and
it can be inspected with the `journalctl(1)` command.

The user can disable all background updates (either started through
timers or not) by executing the following command:

`systemctl mask swupd-update.service`

## ENVIRONMENT

This unit is a `systemd(1)` unit file.

## COPYRIGHT

 * Copyright (C) 2016 Intel Corporation, License: CC-BY-SA-3.0

## SEE ALSO

`swupd(1)`, `swupd-update.timer(4)`

## NOTES

Creative Commons Attribution-ShareAlike 3.0 Unported

 * http://creativecommons.org/licenses/by-sa/3.0/
