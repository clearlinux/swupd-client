swupd-update.timer(4) -- System Unit
==================================

## SYNOPSIS

`swupd-update.timer`
`/usr/lib/systemd/system/swupd-update.timer`

## DESCRIPTION

Instructs the `systemd(1)` system daemon when to periodically start a
system software update. The update itself may not execute if the
`swupd-update.service(4)` is disabled. See that manual page for
information on how to disable that unit.

## ENVIRONMENT

This unit is a `systemd(1)` unit file.

## COPYRIGHT

 * Copyright (C) 2016 Intel Corporation, License: CC-BY-SA-3.0

## SEE ALSO

`swupd(1)`, `swupd-update.service(4)`

## NOTES

Creative Commons Attribution-ShareAlike 3.0 Unported

 * http://creativecommons.org/licenses/by-sa/3.0/
