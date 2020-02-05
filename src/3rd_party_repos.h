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

/** @brief Name of the directory where 3rd-party content and state should be stored */
#define SWUPD_3RD_PARTY_DIRNAME "3rd-party"

/** @brief Full path to the main directory for 3rd-party */
#define SWUPD_3RD_PARTY_DIR "/opt/" SWUPD_3RD_PARTY_DIRNAME

/** @brief Full path to the directory where 3rd-party content is going to be installed. */
#define SWUPD_3RD_PARTY_BUNDLES_DIR SWUPD_3RD_PARTY_DIR "/bundles"

/** @brief Full path to the directory where 3rd-party binaries are going to be installed */
#define SWUPD_3RD_PARTY_BIN_DIR SWUPD_3RD_PARTY_DIR "/bin"

/** @brief Store information of a repository.  */
struct repo {
	/** @brief repo's name */
	char *name;
	/** @brief repo's url */
	char *url;
};

/** @brief Definition of a function that performs an action on a given bundle */
typedef enum swupd_code (*run_operation_fn_t)(char *bundle);

/** @brief Function that returns the path to the 3rd-party bin directory */
char *third_party_get_bin_dir(void);

/** @brief Function that returns the path to a 3rd-party binary */
char *third_party_get_binary_path(const char *binary_name);

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
int third_party_remove_repo(const char *repo_name);

/**
 * @brief This function removes the directory where the content of a repo is installed.
 *
 * @param repo_name A string containing repo_name
 *
 * @returns 0 on success, otherwise a negative errno on any failure
 */
int third_party_remove_repo_directory(const char *repo_name);

/**
 * @brief This function sets up swupd to use a specific 3rd-party repo.
 *
 * @param state_dir the original state directory of the system
 * @param path_prefix the original path prefix of the system
 * @param sigcheck indicates if the repo certificate has to be validate or not
 *
 * @returns a swupd_code
 */
enum swupd_code third_party_set_repo(struct repo *repo, bool sigcheck);

/**
 * @brief Performs an operation on every bundle from the list.
 *
 * @param bundles the list of bundles to which the operation will be performed
 * @param repo the name of the 3rd-party repository where the bundles
 * should be looked for. If no repo is specified the bundles are searched for
 * in all existing repos.
 * @param run_operation_fn the function to be performed
 *
 * @returns a swupd_code
 */
enum swupd_code third_party_run_operation(struct list *bundles, const char *repo, run_operation_fn_t run_operation_fn);

/**
 * @brief Performs an operation on 3rd-party repos.
 *
 * @param repo the name of the 3rd-party repository where the operation will be run
 * @param run_operation_fn the function to be performed
 * @param expected_ret_code the expected return code from the operation
 * @param op_name the name of the operation to be run in all repos
 * @param op_steps the number of steps involved in the operation
 *
 * @returns a swupd_code
 */
enum swupd_code third_party_run_operation_multirepo(const char *repo, run_operation_fn_t run_operation_fn, enum swupd_code expected_ret_code, const char *op_name, int op_steps);

/**
 * @brief Prints a header with the repository name, useful when showing info from multiple repos.
 *
 * @param repo_name, the name of the 3rd-party repository
 */
void third_party_repo_header(const char *repo_name);

#endif

#ifdef __cplusplus
}
#endif

#endif
