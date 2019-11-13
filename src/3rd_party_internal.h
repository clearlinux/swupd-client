#ifndef __THIRD_PARTY_INTERNAL_H
#define __THIRD_PARTY_INTERNAL_H

#include "swupd.h"
#include <config.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @file
 * @brief Common functions which can be used by 3rd-party add,
 * remove, list
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef THIRDPARTY
static inline char *get_repo_path(char *repo_name)
{
	char *third_party_temp = sys_path_join(globals.path_prefix, THIRDPARTY_REPO_PREFIX);
	char *abs_third_party_directory = sys_path_join(third_party_temp, repo_name);
	free_string(&third_party_temp);
	return abs_third_party_directory;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
