===========
swupd-alias
===========

-------------------------------------
Alias Names for Bundles Configuration
-------------------------------------

:Copyright: \(C) 2018 Intel Corporation, CC-BY-SA-3.0
:Manual section: 7


DESCRIPTION
===========

``swupd bundle-add`` now understands alias names in addition to bundle names.
These alias names are defined in files contained in two directories, the system
directory /usr/share/defaults/swupd/alias.d/ and the user directory /etc/swupd/alias.d/.

The file format for the alias file is a potentially multiline file with an
alias name per line followed by, on the same line, a list of bundle names
with all fields being tab separated.

For example:

alias-name1<tab>bundle-one<tab>bundle-two<newline>
alias-name2<tab>bundle-three

Would be an alias defintion file with two aliases, alias-name1 and alias-name2
defined in it. Running ``swupd bundle-add alias-name1`` would install bundle-one
and bundle-two.

NOTES
=====

Alias definitions can be defined multiple times in a file, if this happens the
first definition will be used.

Alias files in the user directory are loaded before alias files in the system
directory. Because of this, if an alias is defined in both a user and system
file, the user defined alias will be used instead of the system defined alias.

Alias files are loaded in lexicographical filename order and that implies if an
alias is defined in more than one file, the first file to be loaded will be the
one whose alias is used.

Alias files in the user directory override or mask files with the same name in
the system directory. If a user wishes to disable a system alias file they can
create a symlink to /dev/null for the corresponding user file and the system
alias file will not be used.

SEE ALSO
--------

* ``swupd``\(1)
* https://github.com/clearlinux/swupd-client/
* https://clearlinux.org/documentation/
