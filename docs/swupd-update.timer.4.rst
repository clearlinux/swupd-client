==================
swupd-update.timer
==================

----------------------------
Schedules periodical updates
----------------------------

:Copyright: \(C) 2017-2025 Intel Corporation, CC-BY-SA-3.0
:Manual section: 4


SYNOPSIS
========

``swupd-update.timer``

``/usr/lib/systemd/system/swupd-update.timer``


DESCRIPTION
===========

Instructs the ``systemd``\(1) system daemon when to periodically start a
system software update. The update itself may not execute if the
``swupd-update.service``\(4) is disabled. See that manual page for
information on how to disable that unit.


ENVIRONMENT
===========

This unit is a ``systemd``\(1) unit file.


SEE ALSO
========

``swupd``\(1), ``swupd-update.service``\(4)

