=================
swupd-add-pkg
=================

------------------------------------
Adds custom package for swupd to use
------------------------------------

:Copyright: \(C) 2017 Intel Corporation, CC-BY-SA-3.0
:Manual section: 4


SYNOPSIS
========

``swupd-add-pkg``

``/usr/bin/swupd-add-pkg``


DESCRIPTION
===========

This tool requires ``sudo`` access to run.

Adds a given local package to the specified <bundle_name> file, and
automatically generates all content with mixer for swupd to use. It creates
a very minimalist mix, unlike regular mixer usage where one composes the
entire OS. This instead outputs only the new bundle content, and os-core -
to signify the version changes - in order to augment the regular upstream
update content. Swupd is aware of this new user added content, and if it
exists on the system, swupd will attempt to use it for any of its operations,
e.g update, verify, bundle add, etc.

While this changes the version of the OS, note that it is the current version
multiplied by 1000. This is so the base, upstream version is easily determined,
and it allows for up to 100 new package adds in between upstream releases.


ENVIRONMENT
===========

This tool is part of the swupd software update package.


SEE ALSO
========

* ``swupd``\(1)
* https://clearlinux.org/documentation/clear-linux/guides/maintenance/swupd-mixer-integration.html
