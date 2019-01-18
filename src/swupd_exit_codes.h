#ifndef __INCLUDE_GUARD_SWUPD_ERROR_H
#define __INCLUDE_GUARD_SWUPD_ERROR_H

/*
 * Return codes used by swupd
 * Always add new codes last
*/

typedef enum {

	SWUPD_OK = 0,			/* 0 success */
	SWUPD_NO,			/* 1 used by swupd to respond "No" in different situations:
                                          - if used with check-update it means no update is available
                                          - if used with autoupdate it means automatic updating is disabled*/
	SWUPD_COULDNT_LOAD_MOM,		/* 2 MoM cannot be loaded into memory (this could imply network issue) */
	SWUPD_COULDNT_REMOVE_FILE,      /* 3 couldn't delete a file which must be deleted */
	SWUPD_COULDNT_OVERWRITE_DIR,    /* 4 couldn't overwrite a directory */
	SWUPD_COULDNT_CREATE_DOTFILE,   /* 5 couldn't create a dotfile */
	SWUPD_RECURSE_MANIFEST,		/* 6 error while recursing a manifest */
	SWUPD_LOCK_FILE_FAILED,		/* 7 cannot get the lock */
	SWUPD_CURL_INIT_FAILED,		/* 8 cannot initialize curl agent */
	SWUPD_INIT_GLOBALS_FAILED,      /* 9 cannot initialize globals */
	SWUPD_BUNDLE_NOT_TRACKED,       /* 10 bundle is not tracked on the system */
	SWUPD_COULDNT_LOAD_MANIFEST,    /* 11 cannot load manifest into memory */
	SWUPD_INVALID_OPTION,		/* 12 invalid command option */
	SWUPD_SERVER_CONNECTION_ERROR,  /* 13 no network connection to swupd server */
	SWUPD_COULDNT_DOWNLOAD_FILE,    /* 14 file download problem */
	SWUPD_COULDNT_INSTALL_BUNDLE,   /* 15 cannot install bundles */
	SWUPD_COULDNT_CREATE_DIRS,      /* 16 cannot create required directories */
	SWUPD_CURRENT_VERSION_UNKNOWN,  /* 17 cannot determine current OS version */
	SWUPD_COULDNT_VERIFY_SIGNATURE, /* 18 cannot initialize signature verification */
	SWUPD_BAD_TIME,			/* 19 system time is bad */
	SWUPD_COULDNT_DOWNLOAD_PACK,    /* 20 pack download failed */
	SWUPD_BAD_CERT,			/* 21 unable to verify server SSL certificate */
	SWUPD_REQUIRED_BUNDLE_ERROR,    /* 22 a required bundle is missing or was attempted to be removed */
	SWUPD_INVALID_BUNDLE		/* 23 the specified bundle is invalid */

} swupd_code;

#endif
