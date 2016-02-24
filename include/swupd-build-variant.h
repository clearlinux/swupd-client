#ifndef __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H
#define __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H

#include <dirent.h>
#include <stdbool.h>

#include "config.h"
#include "list.h"
#include "swupd.h"

#ifdef SWUPD_WITH_SELINUX
#define TAR_PERM_ATTR_ARGS "--preserve-permissions --xattrs --xattrs-include='*' --selinux"
#else /* SWUPD_WITHOUT_SELINUX */
#define TAR_PERM_ATTR_ARGS "--preserve-permissions --xattrs --xattrs-include='*'"
#endif

#endif
