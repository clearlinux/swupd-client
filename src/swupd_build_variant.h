#ifndef __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H
#define __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H

#include <dirent.h>
#include <stdbool.h>

#include "lib/list.h"
#include "swupd.h"

#ifdef SWUPD_WITH_BSDTAR

/* bsdtar always includes xattrs, without needing special parameters
 * for it. This has been tested in Ostro OS in combination with IMA
 * and Smack, but not with SELinux.
 */
#define TAR_COMMAND "/usr/bin/bsdtar"

#else /* SWUPD_WITH_BSDTAR */

/* GNU tar requires special options, depending on how the OS uses xattrs.
 * This has been tested in Clear Linux OS in combination with SELinux.
 */
#define TAR_COMMAND "/usr/bin/tar"

#ifdef SWUPD_WITH_XATTRS

#ifdef SWUPD_TAR_SELINUX
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*' --selinux"
#else /* SWUPD_TAR_SELINUX */
#define TAR_XATTR_ARGS "--xattrs --xattrs-include='*'"
#endif /* SWUPD_TAR_SELINUX */

#endif /* SWUPD_WITH_XATTRS */

#endif /* SWUPD_WITH_BSDTAR */

#define TAR_PERM_ATTR_ARGS "--preserve-permissions"

#endif /* __INCLUDE_GUARD_SWUPD_BUILD_VARIANT_H */
