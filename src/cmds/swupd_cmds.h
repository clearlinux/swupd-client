#ifndef __SWUPD_CMDS__
#define __SWUPD_CMDS__

/**
 * @file
 * @brief Main functions used by swupd commands. Not to be used unless you
 * are a calling a main() of a sub-command.
 */

#include <stddef.h>
#include <string.h>

#include "lib/strings.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main function of command autoupdate.
 */
enum swupd_code autoupdate_main(int argc, char **argv);

/**
 * @brief Main function of command bundle-add.
 */
enum swupd_code bundle_add_main(int argc, char **argv);

/**
 * @brief Main function of command bundle-remove.
 */
enum swupd_code bundle_remove_main(int argc, char **argv);

/**
 * @brief Main function of command bundle-list.
 */
enum swupd_code bundle_list_main(int argc, char **argv);

/**
 * @brief Main function of command hashdump.
 */
enum swupd_code hashdump_main(int argc, char **argv);

/**
 * @brief Main function of command update.
 */
enum swupd_code update_main(int argc, char **argv);

/**
 * @brief Main function of command check-update.
 */
enum swupd_code check_update_main(int argc, char **argv);

/**
 * @brief Main function of command search-file.
 */
enum swupd_code search_file_main(int argc, char **argv);

/**
 * @brief Main function of command info.
 */
enum swupd_code info_main(int argc, char **argv);

/**
 * @brief Main function of command clean.
 */
enum swupd_code clean_main(int argc, char **argv);

/**
 * @brief Main function of command mirror.
 */
enum swupd_code mirror_main(int argc, char **argv);

/**
 * @brief Main function of commands that uses binary-loader.
 */
enum swupd_code binary_loader_main(int argc, char **argv);

/**
 * @brief Main function of command os-install.
 */
enum swupd_code install_main(int argc, char **argv);

/**
 * @brief Main function of command repair.
 */
enum swupd_code repair_main(int argc, char **argv);

/**
 * @brief Main function of command diagnose.
 */
enum swupd_code diagnose_main(int argc, char **argv);

/**
 * @brief Main function of command bundle-info.
 */
enum swupd_code bundle_info_main(int argc, char **argv);

/**
 * @brief Main function of command verify.
 */
enum swupd_code verify_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party.
 */
enum swupd_code third_party_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party info.
 */
enum swupd_code third_party_info_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party bundle-add.
 */
enum swupd_code third_party_bundle_add_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party bundle-list.
 */
enum swupd_code third_party_bundle_list_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party bundle-remove.
 */
enum swupd_code third_party_bundle_remove_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party bundle-info.
 */
enum swupd_code third_party_bundle_info_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party update.
 */
enum swupd_code third_party_update_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party diagnose.
 */
enum swupd_code third_party_diagnose_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party repair.
 */
enum swupd_code third_party_repair_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party check-update.
 */
enum swupd_code third_party_check_update_main(int argc, char **argv);

/**
 * @brief Main function of command 3rd-party clean.
 */
enum swupd_code third_party_clean_main(int argc, char **argv);

/**
 * @brief Creates a new third-party repo under THIRDPARTY_REPO_PREFIX
 * if it does not exist
 *
 * @return SWUPD_OK on success
 */
extern enum swupd_code third_party_add_main(int argc, char **argv);

/**
 * @brief Removes a third-party repo under THIRDPARTY_REPO_PREFIX
 * if it does exist
 *
 * @return SWUPD_OK on success
 */
extern enum swupd_code third_party_remove_main(int argc, char **argv);

/**
 * @brief Lists all the third-party repos under THIRDPARTY_REPO_PREFIX
 *
 * @return SWUPD_OK on success
 */
extern enum swupd_code third_party_list_main(int argc, char **argv);

/**
 * @brief Struct to hold a command specification.
 */
struct subcmd {
	/** @brief The name of the command */
	char *name;
	/** @brief Documentation string to be used on --help */
	char *doc;
	/** @brief Function to be executed by the command */
	enum swupd_code (*mainfunc)(int, char **);
};

/**
 * @brief Look for a arg in a list of commands
 */
static inline int subcmd_index(char *arg, struct subcmd *commands)
{
	struct subcmd *entry = commands;
	int i = 0;

	while (entry->name != NULL) {
		if (str_cmp(arg, entry->name) == 0) {
			return i;
		}
		entry++;
		i++;
	}

	return -1;
}

#ifdef __cplusplus
}
#endif

#endif
