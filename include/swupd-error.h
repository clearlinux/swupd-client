#ifndef __INCLUDE_GUARD_SWUPD_ERROR_H
#define __INCLUDE_GUARD_SWUPD_ERROR_H

#define EBUNDLE_MISMATCH 2      /* at least one local bundle mismatches from MoM */
#define EBUNDLE_REMOVE 3	/* cannot delete local bundle filename */
#define EMOM_NOTFOUND 4		/* MoM cannot be loaded into memory (this could imply network issue) */
#define ETYPE_CHANGED_FILE_RM 5 /* do_staging() couldn't delete a file which must be deleted */
#define EDIR_OVERWRITE 6	/* do_staging() couldn't overwrite a directory */
#define EDOTFILE_WRITE 7	/* do_staging() couldn't create a dotfile */
#define ERECURSE_MANIFEST 8     /* error while recursing a manifest */
#define ELOCK_FILE 9		/* cannot get the lock */
#define EPREP_MOUNT 10		/* failed to prepare mount points */
#define ECURL_INIT 11		/* cannot initialize curl agent */
#define EINIT_GLOBALS 12	/* cannot initialize globals */
#define EBUNDLE_NOT_TRACKED 13  /* bundle is not tracked on the system */
#define EMANIFEST_LOAD 14       /* cannot load manifest into memory */
#define EINVALID_OPTION 15      /* invalid command option */
#define ENOSWUPDSERVER 16       /* no net connection to swupd server */
#define EFULLDOWNLOAD 17	/* full_download problem */
#define ENET404 404		/* download 404'd */
#define EBUNDLE_INSTALL 18      /* Cannot install bundles */
#define EREQUIRED_DIRS 19       /* Cannot create required dirs */
#define ECURRENT_VERSION 20     /* Cannot determine current OS version */
#define ESIGNATURE 21		/* Cannot initialize signature verification */
#define EBADTIME 22		/* System time is bad */

#endif
