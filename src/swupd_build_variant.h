#ifndef __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H
#define __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H

#include <dirent.h>
#include <stdbool.h>

#include "config.h"
#include "lib/list.h"
#include "swupd.h"

#ifdef SWUPD_WITH_BSDTAR

/* bsdtar always includes xattrs, without needing special parameters
 * for it. This has been tested in Ostro OS in combination with IMA
 * and Smack, but not with SELinux.
 */
#define TAR_COMMAND "bsdtar"
#define TAR_XATTR_ARGS ""

#else /* SWUPD_WITH_BSDTAR */

/* GNU tar requires special options, depending on how the OS uses xattrs.
 * This has been tested in Clear Linux OS in combination with SELinux.
 */
#define TAR_COMMAND "tar"
#ifdef SWUPD_TAR_SELINUX
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*' --selinux"
#else
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*'"
#endif

#endif /* SWUPD_WITH_BSDTAR */

#ifdef SWUPD_WITH_XATTRS
#define TAR_PERM_ATTR_ARGS "--preserve-permissions " TAR_XATTR_ARGS
#else /* SWUPD_WITHOUT_XATTRS */
#define TAR_PERM_ATTR_ARGS "--preserve-permissions "
#endif

#endif
