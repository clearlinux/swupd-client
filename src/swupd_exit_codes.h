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
	SWUPD_COULDNT_CREATE_DIRS,      /* 15 cannot create required directories */
	SWUPD_CURRENT_VERSION_UNKNOWN,  /* 16 cannot determine current OS version */
	SWUPD_COULDNT_VERIFY_SIGNATURE, /* 17 cannot initialize signature verification */
	SWUPD_BAD_TIME,			/* 18 system time is bad */
	SWUPD_COULDNT_DOWNLOAD_PACK,    /* 19 pack download failed */
	SWUPD_BAD_CERT,			/* 20 unable to verify server SSL certificate */
	SWUPD_REQUIRED_BUNDLE_ERROR,    /* 21 a required bundle is missing or was attempted to be removed */
	SWUPD_INVALID_BUNDLE,		/* 22 the specified bundle is invalid */
	SWUPD_DISK_SPACE_ERROR,		/* 23 not enough disk space left (or it cannot be determined) */
	SWUPD_COULDNT_UNTAR_FILE,       /* 24 couldn't untar a file */
	SWUPD_PATH_NOT_IN_MANIFEST,     /* 25 the required path is not in any manifest */
	SWUPD_UNEXPECTED_CONDITION,     /* 26 an unexpected condition was found */
	SWUPD_COULDNT_RENAME_DIR,       /* 27 couldn't rename a directory */
	SWUPD_COULDNT_RENAME_FILE,      /* 28 couldn't rename a file */
	SWUPD_SUBPROCESS_ERROR,		/* 29 failure to execute another program in a subprocess */
	SWUPD_COULDNT_LIST_DIR,		/* 30 couldn't list the content of a directory */
	SWUPD_COMPUTE_HASH_ERROR,       /* 31 there was an error computing the hash of the specified file */
	SWUPD_COULDNT_GET_TIME,		/* 32 couldn't get current system time */
	SWUPD_WRITE_FILE_ERROR,		/* 33 there was a an error while creating/writing to a file */
	SWUPD_MIX_COLLISIONS		/* 34 collisions were found between mix and upstream */

} swupd_code;

#endif
