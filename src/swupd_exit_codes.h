#ifndef __INCLUDE_GUARD_SWUPD_ERROR_H
#define __INCLUDE_GUARD_SWUPD_ERROR_H

/*
 * Return codes used by swupd
 * Always add new codes last
*/

enum swupd_code {

	SWUPD_OK = 0,			     /* 0 success */
	SWUPD_NO,			     /* 1 used by swupd to respond "No" in different situations:
                                          - if used with check-update it means no update is available
                                          - if used with autoupdate it means automatic updating is disabled */
	SWUPD_REQUIRED_BUNDLE_ERROR,	 /* 2 a required bundle is missing or was attempted to be removed */
	SWUPD_INVALID_BUNDLE,		     /* 3 the specified bundle is invalid */
	SWUPD_COULDNT_LOAD_MOM,		     /* 4 MoM cannot be loaded into memory (this could imply network issue) */
	SWUPD_COULDNT_REMOVE_FILE,	   /* 5 couldn't delete a file which must be deleted */
	SWUPD_COULDNT_RENAME_DIR,	    /* 6 couldn't rename a directory */
	SWUPD_COULDNT_CREATE_FILE,	   /* 7 couldn't create a file */
	SWUPD_RECURSE_MANIFEST,		     /* 8 error while recursing a manifest */
	SWUPD_LOCK_FILE_FAILED,		     /* 9 cannot get the lock */
	SWUPD_COULDNT_RENAME_FILE,	   /* 10 couldn't rename a file */
	SWUPD_CURL_INIT_FAILED,		     /* 11 cannot initialize curl agent */
	SWUPD_INIT_GLOBALS_FAILED,	   /* 12 cannot initialize globals */
	SWUPD_BUNDLE_NOT_TRACKED,	    /* 13 bundle is not tracked on the system */
	SWUPD_COULDNT_LOAD_MANIFEST,	 /* 14 cannot load manifest into memory */
	SWUPD_INVALID_OPTION,		     /* 15 invalid command option */
	SWUPD_SERVER_CONNECTION_ERROR,       /* 16 no network connection to swupd server */
	SWUPD_COULDNT_DOWNLOAD_FILE,	 /* 17 file download problem */
	SWUPD_COULDNT_UNTAR_FILE,	    /* 18 couldn't untar a file */
	SWUPD_COULDNT_CREATE_DIR,	    /* 19 cannot create required directory */
	SWUPD_CURRENT_VERSION_UNKNOWN,       /* 20 cannot determine current OS version */
	SWUPD_SIGNATURE_VERIFICATION_FAILED, /* 21 cannot initialize signature verification */
	SWUPD_BAD_TIME,			     /* 22 system time is bad */
	SWUPD_COULDNT_DOWNLOAD_PACK,	 /* 23 pack download failed */
	SWUPD_BAD_CERT,			     /* 24 unable to verify server SSL certificate */
	SWUPD_DISK_SPACE_ERROR,		     /* 25 not enough disk space left (or it cannot be determined) */
	SWUPD_PATH_NOT_IN_MANIFEST,	  /* 26 the required path is not in any manifest */
	SWUPD_UNEXPECTED_CONDITION,	  /* 27 an unexpected condition was found */
	SWUPD_SUBPROCESS_ERROR,		     /* 28 failure to execute another program in a subprocess */
	SWUPD_COULDNT_LIST_DIR,		     /* 29 couldn't list the content of a directory */
	SWUPD_COMPUTE_HASH_ERROR,	    /* 30 there was an error computing the hash of the specified file */
	SWUPD_TIME_UNKNOWN,		     /* 31 couldn't get current system time */
	SWUPD_COULDNT_WRITE_FILE,	    /* 32 couldn't write to a file */
	SWUPD_MIX_COLLISIONS,		     /* 33 collisions were found between mix and upstream */
	SWUPD_OUT_OF_MEMORY_ERROR,	   /* 34 swupd ran out of memory */
	SWUPD_VERIFY_FAILED		     /* 35 verify could not fix/replace/delete one or more files */

};

#endif
