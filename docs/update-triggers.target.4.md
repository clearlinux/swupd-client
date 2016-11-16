update-triggers.target(4) -- System Unit
===================================

## SYNOPSIS

`update-triggers.target`
`/usr/lib/systemd/system/update-triggers.target`

## DESCRIPTION

Instructs the `systemd(1)` system daemon what units needed to be started
as part of the software update process. After the main software update
content is applied to the OS, the `swupd(1)` program starts this
*systemd* target.

The main function of this target is to allow optional corrections
and adjustments to be made after installation that can not easily be
made through static file installation methods. This usually applies to
things like cached lists, dynamic configuration files and similar
volatile databases that are regularly altered and may contain user
data.

One example is the dynamic loader cache which needs to be updated when
ever there are modifications to the system installed dynamic libraries.

The user is not intended to execute these scripts and plugin units
directly. If however so this is desired, one can execute the following
command to re-execute all the actions:

`systemctl start update-triggers.target`

## ENVIRONMENT

This unit is a `systemd(1)` target file.

## COPYRIGHT

 * Copyright (C) 2016 Intel Corporation, License: CC-BY-SA-3.0

## SEE ALSO

`swupd(1)`

## NOTES

Creative Commons Attribution-ShareAlike 3.0 Unported

 * http://creativecommons.org/licenses/by-sa/3.0/
