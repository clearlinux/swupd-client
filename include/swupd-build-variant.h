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
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*'"
#endif

#ifdef SWUPD_WITH_SELINUX
#define TAR_PERM_ATTR_ARGS "--preserve-permissions --selinux " TAR_XATTR_ARGS
#else /* SWUPD_WITHOUT_SELINUX */
#define TAR_PERM_ATTR_ARGS "--preserve-permissions " TAR_XATTR_ARGS
#endif

#endif
