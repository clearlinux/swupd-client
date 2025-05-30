====================
swupd-update.service
====================

---------------------
Performs an OS update
---------------------

:Copyright: \(C) 2017-2025 Intel Corporation, CC-BY-SA-3.0
:Manual section: 4


SYNOPSIS
========

``swupd-update.service``

``/usr/lib/systemd/system/swupd-update.service``


DESCRIPTION
===========

Instructs the ``systemd``\(1) system daemon how to start and control the
``swupd`` service to perform an update.

When this unit runs, the output will be sent to the systemd journal, and
it can be inspected with the ``journalctl``\(1) command.

The user can disable all background updates (either started through
timers or not) by executing the following command:

``systemctl mask swupd-update.service``


ENVIRONMENT
===========

This unit is a ``systemd``\(1) unit file.


SEE ALSO
========

``swupd``\(1),  ``swupd-update.timer``\(4)

