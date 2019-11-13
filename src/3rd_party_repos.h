#ifndef __THIRD_PARTY_CONFIG_H
#define __THIRD_PARTY_CONFIG_H

#include "swupd.h"

/**
 * @file
 * @brief 3rd party config file parsing functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef THIRDPARTY

/** @brief Store information of a repository.  */
struct repo {
	char *name;
	char *url;
	char *version;
};

/**
 * @brief Free repo pointed by @c repo.
 */
void repo_free(struct repo *repo);

/**
 * @brief Free repo pointed by @c data.
 *
 * Just a wrapper on repo_free() to help with casting.
 */
void repo_free_data(void *data);

/**
 * @brief Get a list of repos in the repo.ini config file.
 */
struct list *third_party_get_repos(void);

/**
 * @brief This function performs the repo config operation part for repo add
 * It checks if a repo with repo_name already exists and if not creates an
 * entry in the repo config file.
 *
 * @param repo_name A string containing repo_name
 * @param repo_url A string containing repo_url
 *
 * @returns 0 on success ie: a repo with the name is successfully added to repo config
 * otherwise a -1 on any failure
 */
int third_party_add_repo(const char *repo_name, const char *repo_url);

/**
 * @brief This function performs the repo config operation part for repo remove
 * Its function is lookup the repo_name in the repo.ini file and if it exists, remove it
 * and adjust the repo config file.
 *
 * @param repo_name A string containing repo_name
 *
 * @returns 0 on success ie: a repo is found, removed & repo config is adjusted
 * otherwise a -1 on any failure
 */
int third_party_remove_repo(char *repo_name);

#endif

#ifdef __cplusplus
}
#endif

#endif
