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

#include <fcntl.h>
#include <sys/stat.h>

#include "3rd_party_repos.h"
#include "swupd.h"

#ifdef THIRDPARTY

#define FLAG_SKIP_OPTIONAL 2000
#define FLAG_SKIP_DISKSPACE_CHECK 2001

static char **cmdline_bundles;
static char *cmdline_repo = NULL;

typedef enum swupd_code (*proc_binary_fn_t)(const char *filename);

static void print_help(void)
{
	print("Installs new software bundles from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   --skip-optional         Do not install optional bundles (also-add flag in Manifests)\n");
	print("   --skip-diskspace-check  Do not check free disk space before adding bundle\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "skip-optional", no_argument, 0, FLAG_SKIP_OPTIONAL },
	{ "skip-diskspace-check", no_argument, 0, FLAG_SKIP_DISKSPACE_CHECK },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case FLAG_SKIP_OPTIONAL:
		globals.skip_optional_bundles = optarg_to_bool(optarg);
		return true;
	case FLAG_SKIP_DISKSPACE_CHECK:
		globals.skip_diskspace_check = optarg_to_bool(optarg);
		return true;
	case 'R':
		cmdline_repo = strdup_or_die(optarg);
		return true;
	default:
		return false;
	}
	return false;
}

static const struct global_options opts = {
	prog_opts,
	sizeof(prog_opts) / sizeof(struct option),
	parse_opt,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int optind = global_parse_options(argc, argv, &opts);

	if (optind < 0) {
		return false;
	}

	if (argc <= optind) {
		error("missing bundle(s) to be installed\n\n");
		return false;
	}

	cmdline_bundles = argv + optind;

	return true;
}

static bool is_binary(const char *filename)
{
	if (strncmp(filename, "/bin", strlen("/bin")) == 0 ||
	    strncmp(filename, "/usr/bin", strlen("/usr/bin")) == 0 ||
	    strncmp(filename, "/usr/local/bin", strlen("/usr/local/bin")) == 0) {
		return true;
	}

	return false;
}

static enum swupd_code create_wrapper_script(const char *filename)
{
	enum swupd_code ret_code = 0;
	int fd;
	FILE *fp = NULL;
	char *bin_directory = NULL;
	char *script = NULL;
	char *binary = NULL;
	char *env_path = NULL;
	char *env_ld_path = NULL;
	char *third_party_bin_path = NULL;
	char *third_party_ld_path = NULL;
	mode_t mode = 0755;

	/* the path_prefix has the path to the 3rd-party repo, like:
	 *  /<original_path_prefix>/opt/3rd-party/bundles/<repo>/
	 *  we can use it to find the path to the bin directory:
	 *  /<original_path_prefix>/opt/3rd-party/bin */
	char *tmp1 = sys_dirname(globals.path_prefix);
	char *tmp2 = sys_dirname(tmp1);
	bin_directory = sys_path_join(tmp2, "/bin");
	free_string(&tmp1);
	free_string(&tmp2);

	script = sys_path_join(bin_directory, sys_basename(filename));
	binary = sys_path_join(globals.path_prefix, filename);

	if (!sys_filelink_is_executable(binary)) {
		goto close_and_exit;
	}

	if (sys_file_exists(script)) {
		/* the binary already exists, this condition should never happen
		 * since we checked before installing */
		ret_code = SWUPD_UNEXPECTED_CONDITION;
		error("There is already a binary called %s in %s, it will be skipped\n", sys_basename(filename), bin_directory);
		goto close_and_exit;
	}

	/* if the SWUPD_3RD_PARTY_BIN_DIR does not exist, attempt to create it */
	if (!sys_file_exists(bin_directory)) {
		if (mkdir_p(bin_directory)) {
			ret_code = SWUPD_COULDNT_CREATE_DIR;
			goto close_and_exit;
		}
	}

	if (!is_dir(bin_directory)) {
		error("The path %s for 3rd-party content exists but is not a directory\n", bin_directory);
		ret_code = SWUPD_COULDNT_CREATE_DIR;
		goto close_and_exit;
	}

	/* open the file with mode set to 0755 */
	fd = open(script, O_RDWR | O_CREAT, mode);
	if (fd < 0) {
		error("The file %s failed to be created\n", script);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
		goto close_and_exit;
	}
	fp = fdopen(fd, "w");
	if (!fp) {
		error("The file %s failed to be created\n", script);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
		goto close_and_exit;
	}

	/* get the current values of the environment */
	env_path = getenv("PATH");
	env_ld_path = getenv("LD_LIBRARY_PATH");

	/* get the path for the 3rd-party content */
	third_party_bin_path = str_or_die("%sbin:%susr/bin:%susr/local/bin", globals.path_prefix, globals.path_prefix, globals.path_prefix);
	third_party_ld_path = str_or_die("%susr/lib64:%susr/local/lib64", globals.path_prefix, globals.path_prefix);

	fprintf(fp, "#!/bin/bash\n");
	fprintf(fp, "\n");
	fprintf(fp, "export PATH=%s%s%s\n", third_party_bin_path, env_path ? ":" : "", env_path ? "$PATH" : "");
	fprintf(fp, "export LD_LIBRARY_PATH=%s%s%s\n", third_party_ld_path, env_ld_path ? ":" : "", env_ld_path ? "$LD_LIBRARY_PATH" : "");
	fprintf(fp, "\n");
	fprintf(fp, "%s \"$@\"\n", binary);

close_and_exit:
	if (fp) {
		fclose(fp);
	}
	free_string(&binary);
	free_string(&script);
	free_string(&bin_directory);
	free_string(&third_party_bin_path);
	free_string(&third_party_ld_path);

	return ret_code;
}

static enum swupd_code validate_binaries(const char *filename)
{
	enum swupd_code ret_code = SWUPD_OK;
	char *bin_directory = NULL;
	char *script = NULL;

	char *tmp1 = sys_dirname(globals.path_prefix);
	char *tmp2 = sys_dirname(tmp1);
	bin_directory = sys_path_join(tmp2, "/bin");
	free_string(&tmp1);
	free_string(&tmp2);

	script = sys_path_join(bin_directory, sys_basename(filename));

	/* if a file already exists, report the problem to the user */
	if (sys_file_exists(script)) {
		error("There is already a binary called %s in %s\n", sys_basename(filename), bin_directory);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
	}
	free_string(&script);
	free_string(&bin_directory);

	return ret_code;
}

static enum swupd_code process_bundle_binaries(struct list *installed_files, const char *msg, const char *step, proc_binary_fn_t proc_binary_fn)
{
	enum swupd_code ret, ret_code = SWUPD_OK;
	struct list *iter = NULL;
	struct file *file = NULL;
	char *filename = NULL;
	int count = 0;
	int number_of_files = list_len(installed_files);

	info("%s", msg);

	progress_next_step(step, PROGRESS_BAR);
	for (iter = installed_files; iter; iter = iter->next) {
		file = iter->data;
		filename = file->filename;
		if (file->is_exported && is_binary(filename)) {
			ret = proc_binary_fn(filename);
			if (ret) {
				/* at least one wrapper failed */
				ret_code = ret;
			}
		}
		count++;
		progress_report(count, number_of_files);
	}

	return ret_code;
}

static enum swupd_code export_bundle_binaries(struct list *installed_files)
{
	return process_bundle_binaries(installed_files, "\nExporting 3rd-party bundle binaries...\n", "export_binaries", create_wrapper_script);
}

static enum swupd_code validate_bundle_binaries(struct list *files_to_be_installed)
{
	return process_bundle_binaries(files_to_be_installed, "\nValidating 3rd-party bundle binaries...\n", "validate_binaries", validate_binaries);
}

static enum swupd_code add_bundle(char *bundle)
{
	struct list *bundle_to_install = NULL;
	enum swupd_code ret = SWUPD_OK;

	/* execute_bundle_add expects a list */
	bundle_to_install = list_append_data(bundle_to_install, bundle);

	info("\nBundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n\n");
	globals.no_scripts = true;

	ret = execute_bundle_add(bundle_to_install, validate_bundle_binaries, export_bundle_binaries);

	list_free_list(bundle_to_install);
	return ret;
}

enum swupd_code third_party_bundle_add_main(int argc, char **argv)
{
	struct list *bundles = NULL;
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundleadd = 10;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* initialize swupd */
	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	/*
	 * Steps for 3rd-party bundle-add:
	 *  1) load_manifests
	 *  2) validate_binaries
	 *  3) download_packs
	 *  4) extract_packs
	 *  5) validate_fullfiles
	 *  6) download_fullfiles
	 *  7) extract_fullfiles
	 *  8) install_files
	 *  9) run_postupdate_scripts
	 *  10) export_binaries
	 */
	progress_init_steps("3rd-party-bundle-add", steps_in_bundleadd);

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *cmdline_bundles; ++cmdline_bundles) {
		char *bundle = *cmdline_bundles;
		bundles = list_append_data(bundles, bundle);
	}
	bundles = list_head(bundles);

	/* try installing bundles one by one */
	ret_code = third_party_run_operation(bundles, cmdline_repo, add_bundle);

	list_free_list(bundles);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
