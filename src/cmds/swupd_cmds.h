#ifndef __SWUPD_CMDS__
#define __SWUPD_CMDS__

#include <stddef.h>
#include <string.h>

#include "lib/strings.h"

#ifdef __cplusplus
extern "C" {
#endif

enum swupd_code autoupdate_main(int argc, char **argv);
enum swupd_code bundle_add_main(int argc, char **argv);
enum swupd_code bundle_remove_main(int argc, char **argv);
enum swupd_code bundle_list_main(int argc, char **argv);
enum swupd_code hashdump_main(int argc, char **argv);
enum swupd_code update_main(int argc, char **argv);
enum swupd_code check_update_main(int argc, char **argv);
enum swupd_code search_file_main(int argc, char **argv);
enum swupd_code info_main(int argc, char **argv);
enum swupd_code clean_main(int argc, char **argv);
enum swupd_code mirror_main(int argc, char **argv);
enum swupd_code binary_loader_main(int argc, char **argv);
enum swupd_code install_main(int argc, char **argv);
enum swupd_code repair_main(int argc, char **argv);
enum swupd_code diagnose_main(int argc, char **argv);
enum swupd_code bundle_info_main(int argc, char **argv);
enum swupd_code verify_main(int argc, char **argv);
enum swupd_code third_party_main(int argc, char **argv);
enum swupd_code third_party_info_main(int argc, char **argv);
enum swupd_code third_party_bundle_add_main(int argc, char **argv);
enum swupd_code third_party_bundle_list_main(int argc, char **argv);
enum swupd_code third_party_bundle_remove_main(int argc, char **argv);
enum swupd_code third_party_bundle_info_main(int argc, char **argv);
enum swupd_code third_party_update_main(int argc, char **argv);
enum swupd_code third_party_diagnose_main(int argc, char **argv);
enum swupd_code third_party_repair_main(int argc, char **argv);
enum swupd_code third_party_check_update_main(int argc, char **argv);
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

struct subcmd {
	char *name;
	char *doc;
	enum swupd_code (*mainfunc)(int, char **);
};

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
