====================
check-update.service
====================

----------------------
Performs update checks
----------------------

:Copyright: \(C) 2017 Intel Corporation, CC-BY-SA-3.0
:Manual section: 4


SYNOPSIS
========

``check-update.service``

``/usr/lib/systemd/system/check-update.service``


DESCRIPTION
===========

Instructs the ``systemd``\(1) system daemon how to start and control the
``swupd`` service to perform an update check.

When this unit runs, the output will be sent to the systemd journal, and
it can be inspected with the ``journalctl``\(1) command.


ENVIRONMENT
===========

This unit is a ``systemd``\(1) unit file.


SEE ALSO
========

* ``swupd``\(1)
* ``check-update.timer``\(4)

