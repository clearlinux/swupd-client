#ifndef __INCLUDE_GUARD_SWUPD_ERROR_H
#define __INCLUDE_GUARD_SWUPD_ERROR_H

/* codes to be used as swupd exit status */

typedef enum {

	SWUPD_OK = 0,			/* 0 success */
	SWUPD_NO,			/* 1 used by swupd to respond "No" in different situations:
                                          - if used with check-update it means no update is available
                                          - if used with autoupdate it means automatic updating is disabled*/
	SWUPD_COULDNT_REMOVE_BUNDLE,    /* 2 cannot delete local bundle filename */
	SWUPD_COULDNT_LOAD_MOM,		/* 3 MoM cannot be loaded into memory (this could imply network issue) */
	SWUPD_COULDNT_REMOVE_FILE,      /* 4 couldn't delete a file which must be deleted */
	SWUPD_COULDNT_OVERWRITE_DIR,    /* 5 couldn't overwrite a directory */
	SWUPD_COULDNT_CREATE_DOTFILE,   /* 6 couldn't create a dotfile */
	SWUPD_RECURSE_MANIFEST,		/* 7 error while recursing a manifest */
	SWUPD_LOCK_FILE_FAILED,		/* 8 cannot get the lock */
	SWUPD_CURL_INIT_FAILED,		/* 9 cannot initialize curl agent */
	SWUPD_INIT_GLOBALS_FAILED,      /* 10 cannot initialize globals */
	SWUPD_BUNDLE_NOT_TRACKED,       /* 11 bundle is not tracked on the system */
	SWUPD_COULDNT_LOAD_MANIFEST,    /* 12 cannot load manifest into memory */
	SWUPD_INVALID_OPTION,		/* 13 invalid command option */
	SWUPD_SERVER_CONNECTION_ERROR,  /* 14 no network connection to swupd server */
	SWUPD_COULDNT_DOWNLOAD_FILE,    /* 15 file download problem */
	SWUPD_COULDNT_INSTALL_BUNDLE,   /* 16 cannot install bundles */
	SWUPD_COULDNT_CREATE_DIRS,      /* 17 cannot create required directories */
	SWUPD_CURRENT_VERSION_UNKNOWN,  /* 18 cannot determine current OS version */
	SWUPD_COULDNT_VERIFY_SIGNATURE, /* 19 cannot initialize signature verification */
	SWUPD_BAD_TIME,			/* 20 system time is bad */
	SWUPD_COULDNT_DOWNLOAD_PACK,    /* 21 pack download failed */
	SWUPD_BAD_CERT			/* 22 unable to verify server SSL certificate */

} swupd_code;

#endif
