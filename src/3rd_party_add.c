/*
 *   Software Updater - client side
 *
 *      Copyright © 2019 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include "3rd_party_repos.h"
#include "signature.h"
#include "swupd.h"

#ifdef THIRDPARTY

static void print_help(void)
{
	/* TODO(castulo): we need to change this description to match that of the
	 * documentation once the documentation for this content is added */
	print("Adds a repository from which 3rd-party content can be installed\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party add [repo-name] [repo-URL]\n\n");

	global_print_help();
	print("\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
#define EXACT_ARG_COUNT 2
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	int positional_args = argc - ind;
	/* Ensure that repo add expects only two args: repo-name, repo-url */
	switch (positional_args) {
	case 0:
		error("The positional args: repo-name and repo-url are missing\n\n");
		return false;
	case 1:
		error("The positional args: repo-url is missing\n\n");
		return false;
	case EXACT_ARG_COUNT:
		return true;
	default:
		error("Unexpected arguments\n\n");
		return false;
	}
	return false;
}

static int remove_repo(const char *repo_name)
{
	int ret;

	ret = third_party_remove_repo(repo_name);
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}

	ret = third_party_remove_repo_directory(repo_name);
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

static bool confirm_certificate(const char *cert)
{
	int c;

	info("Importing 3rd-party repository public certificate:\n\n");
	signature_print_info(cert);

	info("\n");
	info("Do you want to accept this certificate? (y/N): ");

	c = tolower(getchar());
	info("\n");

	return c == 'y';
}

static int import_temp_certificate(int version, char *hash)
{
	char *cert_tar, *cert, *url;
	int ret = 0;

	cert_tar = sys_path_join(globals.state_dir, "temp_cert.tar");
	cert = sys_path_join(globals.state_dir, hash);

	url = str_or_die("%s/%d/files/%s.tar", globals.content_url, version, hash);

	ret = swupd_curl_get_file(url, cert_tar);
	if (ret != 0) {
		goto out;
	}

	ret = archives_check_single_file_tarball(cert_tar, hash);
	if (ret != 0) {
		goto out;
	}

	ret = archives_extract_to(cert_tar, globals.state_dir);
	if (ret != 0) {
		goto out;
	}

	if (confirm_certificate(cert)) {
		signature_deinit();
		if (!signature_init(cert, NULL)) {
			signature_deinit();
			ret = -EPERM;
			goto out;
		}
	} else {
		ret = -EACCES;
		goto out;
	}

out:
	unlink(cert_tar);
	unlink(cert);
	free(cert);
	free(cert_tar);
	free(url);
	return ret;
}

static int import_certificate_from_version(int version)
{
	int ret = 0;
	char *os_core, *url, *os_core_tar;
	struct manifest *manifest = NULL;
	struct list *i;
	struct file *cert_file = NULL;

	os_core_tar = sys_path_join(globals.state_dir, "temp_manifest.tar");
	os_core = sys_path_join(globals.state_dir, "Manifest.os-core");
	url = str_or_die("%s/%d/%s", globals.content_url, version, "Manifest.os-core.tar");

	unlink(os_core_tar);
	ret = swupd_curl_get_file(url, os_core_tar);
	if (ret) {
		goto out;
	}

	ret = archives_extract_to(os_core_tar, globals.state_dir);
	if (ret != 0) {
		goto out;
	}

	manifest = manifest_parse("os-core", os_core, false);
	if (!manifest) {
		ret = -EPROTO;
		goto out;
	}

	for (i = manifest->files; i; i = i->next) {
		struct file *f = i->data;
		if (strncmp(f->filename, CERT_PATH, sizeof(CERT_PATH)) == 0) {
			cert_file = f;
			break;
		}
	}

	if (!cert_file) {
		ret = -ENOENT;
		goto out;
	}

	ret = import_temp_certificate(cert_file->last_change, cert_file->hash);

out:
	manifest_free(manifest);
	unlink(os_core_tar);
	unlink(os_core);
	free(os_core);
	free(os_core_tar);
	free(url);
	return ret;
}

static int import_third_party_certificate(void)
{
	char *url;
	int tmp_version;

	url = str_or_die("%s/version/format%s/latest", globals.content_url, globals.format_string);
	tmp_version = get_int_from_url(url);
	free(url);

	if (tmp_version <= 0) {
		return -1;
	}

	return import_certificate_from_version(tmp_version);
}

enum swupd_code third_party_add_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int step_in_third_party_add = 9;
	const char *name, *url;
	struct list *repos = NULL;
	struct repo *repo = NULL;
	bool revert = false;
	int repo_version;
	int ret;
	const bool DONT_VERIFY_CERTIFICATE = false;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		return ret_code;
	}

	/*
	 * Steps for repo add:
	 *  1) add_repo,
	 *  2-9) 8 steps for adding bundle os-core
	 */
	progress_init_steps("third-party-add", step_in_third_party_add);

	/* The last two in reverse are the repo-name, repo-url */
	name = argv[argc - 2];
	url = argv[argc - 1];

	if (!is_url_allowed(url)) {
		ret_code = SWUPD_INVALID_OPTION;
		goto finish;
	}

	/* add the repo configuration to the repo.ini file */
	progress_next_step("add_repo", PROGRESS_UNDEFINED);
	info("Adding 3rd-party repository %s...\n\n", name);
	ret = third_party_add_repo(name, url);
	if (ret) {
		if (ret != -EEXIST) {
			error("Failed to add repository %s to config\n\n", name);
			ret_code = SWUPD_COULDNT_WRITE_FILE;
		} else {
			ret_code = SWUPD_INVALID_OPTION;
		}
		goto finish;
	}

	/* at this point the repo has been added to the repo.ini file */
	repos = third_party_get_repos();
	repo = list_search(repos, name, cmp_repo_name_string);
	if (!repo) {
		/* this should not happen */
		ret_code = SWUPD_UNEXPECTED_CONDITION;
		revert = true;
		goto finish;
	}

	/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
	ret_code = third_party_set_repo(repo, DONT_VERIFY_CERTIFICATE);
	if (ret_code) {
		revert = true;
		goto finish;
	}

	if (globals.sigcheck && !globals.user_defined_cert_path) {
		ret = import_third_party_certificate();
		if (ret < 0) {
			if (ret == -ENOENT) {
				error("Public certificate not found on 3rd-party repository\n");
				info("To ignore certificate check use --nosigcheck\n");
			} else if (ret != -EACCES) {
				error("Impossible to import 3rd party repository certificate\n");
				info("To ignore certificate check use --nosigcheck\n");
			}
			revert = true;

			ret_code = SWUPD_BAD_CERT;
			goto finish;
		}
	}

	/* get repo's latest version */
	repo_version = get_latest_version(repo->url);
	if (repo_version < 0) {
		error("Unable to determine the latest version for repository %s\n\n", repo->name);
		ret_code = SWUPD_INVALID_REPOSITORY;
		revert = true;
		goto finish;
	}

	/* the repo's "os-core" bundle needs to be installed at this moment
	 * so we can track the version of the repo */
	info("Installing the required bundle 'os-core' from the repository...\n");
	info("Note that all bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n");
	globals.no_scripts = true;
	struct list *bundle_to_install = NULL;
	bundle_to_install = list_append_data(bundle_to_install, "os-core");
	ret_code = bundle_add(bundle_to_install, repo_version);
	if (ret_code) {
		revert = true;
	}
	list_free_list(bundle_to_install);

finish:
	if (revert) {
		/* there was an error adding the repo, revert the action,
		 * we don't want to keep a corrupt repo */
		ret = remove_repo(name);
		if (ret) {
			error("The corrupt repository failed to be removed (errno: %d)\n\n", ret);
		}
	}

	if (ret_code == SWUPD_OK && !revert) {
		print("\nRepository added successfully\n");
	} else {
		print("\nFailed to add repository\n");
	}

	list_free_list_and_data(repos, repo_free_data);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
