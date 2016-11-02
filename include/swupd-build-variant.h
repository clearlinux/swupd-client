#ifndef __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H
#define __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H

#include <dirent.h>
#include <stdbool.h>

#include "config.h"
#include "list.h"
#include "swupd.h"

#ifdef SWUPD_WITH_BSDTAR
#define TAR_COMMAND "bsdtar"
#define TAR_XATTR_ARGS ""
#else
#define TAR_COMMAND "tar"
/* configure.ac ensures a sensible configuration of bsdtar/selinux/xattr */
#ifdef SWUPD_TAR_SELINUX
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*' --selinux"
#else
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*'"
#endif
#endif

#ifdef SWUPD_WITH_XATTRS
#define TAR_PERM_ATTR_ARGS "--preserve-permissions " TAR_XATTR_ARGS
#else /* SWUPD_WITHOUT_XATTRS */
#define TAR_PERM_ATTR_ARGS "--preserve-permissions "
#endif

#endif
