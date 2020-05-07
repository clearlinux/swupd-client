/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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
 *   Authors:
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"
#include "swupd_lib/signature.h"
#include "swupd_lib/target_root.h"
#include "swupd_lib/heuristics.h"

#define FLAG_EXTRA_FILES_ONLY 2000
#define FLAG_FILE 2001

static const char picky_tree_default[] = "/usr";
static const char picky_whitelist_default[] = "/usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src";

static bool warning_printed = false;
static bool cmdline_option_download = false;
static bool cmdline_option_skip_optional = false;
static bool cmdline_command_verify = false;
static bool cmdline_option_force = false;
static bool cmdline_option_fix = false;
static bool cmdline_option_picky = false;
static char *cmdline_option_statedir_cache = NULL;
static char *cmdline_option_picky_tree = NULL;
static char *cmdline_option_file = NULL;
static const char *cmdline_option_picky_whitelist = picky_whitelist_default;
static bool cmdline_option_install = false;
static bool cmdline_option_quick = false;
static struct list *cmdline_option_bundles = NULL;
static bool cmdline_option_extra_files_only = false;

/* picky_whitelist points to picky_whitelist_buffer if and only if regcomp() was called for it */
static regex_t *picky_whitelist;

static int version = 0;

/* Count of how many files we managed to not fix */
static struct file_counts counts;

static const struct option prog_opts[] = {
	{ "fix", no_argument, 0, 'f' },
	{ "force", no_argument, 0, 'x' },
	{ "install", no_argument, 0, 'i' },
	{ "version", required_argument, 0, 'V' },
	{ "manifest", required_argument, 0, 'm' },
	{ "picky", no_argument, 0, 'Y' },
	{ "picky-tree", required_argument, 0, 'X' },
	{ "picky-whitelist", required_argument, 0, 'w' },
	{ "quick", no_argument, 0, 'q' },
	{ "bundles", required_argument, 0, 'B' },
	{ "extra-files-only", no_argument, 0, FLAG_EXTRA_FILES_ONLY },
	{ "file", required_argument, 0, FLAG_FILE },
};

/* setter functions */
static void verify_set_command_verify(bool opt)
{
	/* indicates if the superseded "swupd verify" command was used */
	cmdline_command_verify = opt;
}

void verify_set_option_download(bool opt)
{
	cmdline_option_download = opt;
}

void verify_set_option_skip_optional(bool opt)
{
	cmdline_option_skip_optional = opt;
}

void verify_set_option_force(bool opt)
{
	cmdline_option_force = opt;
}

void verify_set_option_install(bool opt)
{
	cmdline_option_install = opt;
}

void verify_set_option_quick(bool opt)
{
	cmdline_option_quick = opt;
}

void verify_set_option_bundles(struct list *opt_bundles)
{
	cmdline_option_bundles = opt_bundles;
}

void verify_set_option_version(int ver)
{
	version = ver;
}

void verify_set_option_statedir_cache(char *path)
{
	FREE(cmdline_option_statedir_cache);
	cmdline_option_statedir_cache = path;
}

void verify_set_option_fix(bool opt)
{
	cmdline_option_fix = opt;
}

void verify_set_option_picky(bool opt)
{
	cmdline_option_picky = opt;
}

void verify_set_picky_whitelist(regex_t *whitelist)
{
	picky_whitelist = whitelist;
}

void verify_set_picky_tree(char *picky_tree)
{
	FREE(cmdline_option_picky_tree);
	cmdline_option_picky_tree = picky_tree;
}

void verify_set_extra_files_only(bool opt)
{
	cmdline_option_extra_files_only = opt;
}

void verify_set_option_file(char *path)
{
	FREE(cmdline_option_file);
	cmdline_option_file = path;
}

static void print_if_verify(const char *option)
{
	if (cmdline_command_verify) {
		print("%s", option);
	}
}

static void print_if_diagnose(const char *option)
{
	if (!cmdline_command_verify) {
		print("%s", option);
	}
}

static void print_help(void)
{
	print("Performs a system software installation verification\n\n");
	print("Usage:\n");
	print_if_verify("   swupd verify [OPTION...]\n\n");
	print_if_verify("Warning: The \"swupd verify\" command has been superseded\n");
	print_if_verify("Please consider using \"swupd diagnose\", \"swupd repair\" or \"swupd os-install\" instead\n\n");
	print_if_diagnose("   swupd diagnose [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -V, --version=[VER]     %s against manifest version VER\n", cmdline_command_verify ? "Verify" : "Diagnose");
	print("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	print("   -q, --quick             Don't check for corrupt files, only fix missing files\n");
	print("   -B, --bundles=[BUNDLES] Forces swupd to only %s the specified BUNDLES. Example: --bundles=xterm,vim\n", cmdline_command_verify ? "verify" : "diagnose");
	print_if_verify("   -m, --manifest=[VER]    This option has been superseded. Please consider using the -V option instead\n");
	print_if_verify("   -f, --fix               This option has been superseded, please consider using \"swupd repair\" instead\n");
	print_if_verify("   -i, --install           This option has been superseded, please consider using \"swupd os-install\" instead\n");
	print_if_verify("   -Y, --picky             Also list (without --fix) or remove (with --fix) files which should not exist\n");
	print_if_diagnose("   -Y, --picky             Also list files which should not exist. By default swupd only looks for them at /usr\n");
	print_if_diagnose("                           skipping /usr/lib/modules, /usr/lib/kernel, /usr/local, and /usr/src\n");
	print("   -X, --picky-tree=[PATH] Changes the path where --picky and --extra-files-only look for extra files\n");
	print("   -w, --picky-whitelist=[RE] Directories that match the regex get skipped during --picky. Example: /usr/bin|/usr/doc\n");
	print("   --extra-files-only      Like --picky, but it only performs this task\n");
	print_if_diagnose("   --file                  Forces swupd to only diagnose the specified file or directory (recursively)\n");
	print("\n");
}

static bool hash_needs_work(struct file *file, char *hash)
{
	if (cmdline_option_quick) {
		return (hash_is_zeros(hash));
	} else {
		return (!hash_equal(file->hash, hash));
	}
}

static int get_all_files(struct manifest *official_manifest, struct list *subs)
{
	int ret;

	/* for install we need everything so synchronously download zero packs */
	ret = download_subscribed_packs(subs, official_manifest, true);
	if (ret < 0) { // require zero pack
		/* If we hit this point, we know we have a network connection, therefore
		 * 	the error is server-side. This is also a critical error, so detailed
		 * 	logging needed */
		error("zero pack downloads failed\n");
		return -SWUPD_COULDNT_DOWNLOAD_PACK;
	}
	return ret;
}

/*
 * Check if the hash of all files in the list matches the system and in this case mark
 * them as do_not_update.
 * If cmdline_option_quick is set, use lazy hash mode and just check if file exists.
 * Return 1 if the hashes of all files in the list matches the hash of the file in
 * the system. Returns 0 otherwise.
 */
static int check_files_hash(struct list *files)
{
	struct list *iter;
	int complete = 0;
	int total = list_len(files);
	int ret = 1;

	info("Checking for corrupt files\n");
	iter = list_head(files);
	while (iter) {
		struct file *f = iter->data;
		char *fullname;
		bool valid;

		complete++;
		iter = iter->next;
		if (f->is_deleted || f->do_not_update) {
			goto progress;
		}

		fullname = sys_path_join("%s/%s", globals.path_prefix, f->filename);
		debug("Verify file - %s\n", f->filename);
		valid = cmdline_option_quick ? verify_file_lazy(fullname) : verify_file(f, fullname);
		FREE(fullname);
		if (valid) {
			f->do_not_update = 1;
		} else {
			ret = 0;
		}
	progress:
		progress_report(complete, total);
	}

	return ret;
}

/* allow optimization of install case */
static int get_required_files(struct manifest *official_manifest, struct list *required_files, struct list *subs)
{
	int ret;

	if (cmdline_option_install) {
		print("\n");
		get_all_files(official_manifest, subs);
	}

	progress_next_step("check_files_hash", PROGRESS_BAR);
	info("\n");
	if (check_files_hash(required_files)) {
		/* we don't need to do these steps, we already have the files,
		 * just complete the steps */
		progress_next_step("validate_fullfiles", PROGRESS_BAR);
		progress_next_step("download_fullfiles", PROGRESS_BAR);
		progress_next_step("extract_fullfiles", PROGRESS_UNDEFINED);
		return 0;
	}

	ret = download_fullfiles(required_files, NULL);
	if (ret) {
		error("Unable to download necessary files for this OS release\n");
	}

	return ret;
}

/*
 * Only called when a file has failed to be fixed during a verify or install.
 * If a low-space warning has been printed, don't check again,
 * but just warn the user and return.
*/
static void check_warn_freespace(const struct file *file)
{
	long fs_free;
	char *original = NULL;
	static bool no_freespace_flag = false;
	struct stat st;

	if (!(cmdline_option_install || cmdline_option_fix)) {
		goto out;
	}

	/* If true, skip expensive operations for future file failures, decreasing time to completion. */
	if (no_freespace_flag) {

		warn("No space left on device\n");
		goto out;
	}

	string_or_die(&original, "%s/staged/%s", globals.state_dir, file->hash);
	fs_free = get_available_space(globals.path_prefix);
	if (fs_free < 0 || stat(original, &st) != 0) {
		warn("Unable to determine free space on filesystem\n");
		goto out;
	}

	if (fs_free < st.st_size * 1.1) {
		warn("File to install (%s) too large by %ldK\n",
		     file->filename, (st.st_size - fs_free) / 1000);
		/* set flag to skip checking space on the second failure, assume we're still out of space */
		no_freespace_flag = true;
	}

out:
	debug("There is enough space on disk for %s\n", original);
	FREE(original);
}

/* for each missing but expected file, (re)add the file */
static void add_missing_files(struct manifest *official_manifest, struct list *files_to_verify, bool repair)
{
	int ret;
	struct file local;
	struct list *iter;
	int list_length = list_len(files_to_verify);
	int complete = 0;

	if (cmdline_option_install) {
		info("Installing base OS and selected bundles\n");
	} else if (cmdline_option_fix) {
		info("Adding any missing files\n");
	} else {
		info("\nChecking for missing files\n");
	}

	iter = list_head(files_to_verify);
	while (iter) {
		struct file *file;
		char *fullname;

		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted || file->do_not_update) {
			goto progress;
		}

		fullname = sys_path_join("%s/%s", globals.path_prefix, file->filename);
		debug("Verify file - %s\n", file->filename);
		memset(&local, 0, sizeof(struct file));
		local.filename = file->filename;
		populate_file_struct(&local, fullname);
		ret = compute_hash_lazy(&local, fullname);
		if (ret != 0) {
			counts.not_replaced++;
			goto out;
		}

		/* compare the hash and report mismatch */
		if (hash_is_zeros(local.hash)) {
			counts.missing++;
			if (!repair || (repair && cmdline_option_install == false)) {
				/* Log to stdout, so we can post-process */
				info(" -> Missing file: ");
				print("%s%s", fullname, repair ? "" : "\n");
			}
		} else {
			goto out;
		}

		/* if not repairing, we're done */
		if (!repair) {
			goto out;
		}

		/* install the new file (on miscompare + fix) */
		if (target_root_install_single_file(file, official_manifest) != SWUPD_OK) {
			debug("target_root_install_single_file for file %s failed\n", file->filename);
		}

		/* verify the hash again to judge success */
		populate_file_struct(&local, fullname);
		if (cmdline_option_quick) {
			ret = compute_hash_lazy(&local, fullname);
		} else {
			ret = compute_hash(&local, fullname);
		}
		if ((ret != 0) || hash_needs_work(file, local.hash)) {
			counts.not_replaced++;
			print(" -> not fixed\n");

			check_warn_freespace(file);

		} else {
			counts.replaced++;
			file->do_not_update = 1;
			if (cmdline_option_install == false) {
				print(" -> fixed\n");
			}
		}
	out:
		FREE(fullname);
	progress:
		progress_report(complete, list_length);
	}
}

static void check_and_fix_one(struct file *file, struct manifest *official_manifest, bool repair)
{
	char *fullname;

	// Note: boot files not marked as deleted are candidates for verify/fix
	if (file->is_deleted || file->do_not_update) {
		return;
	}

	debug("Verify file - %s\n", file->filename);
	/* compare the hash and report mismatch */
	fullname = sys_path_join("%s/%s", globals.path_prefix, file->filename);
	if (verify_file(file, fullname)) {
		goto end;
	}
	// do not account for missing files at this point, they are
	// accounted for in a different stage, only account for mismatch
	if (!sys_file_exists(fullname) != 0) {
		goto end;
	}

	counts.mismatch++;
	/* Log to stdout, so we can post-process it */
	info(" -> Hash mismatch for file: ");
	print("%s%s", fullname, repair ? "" : "\n");

	/* if not repairing, we're done */
	if (!repair) {
		goto end;
	}

	/* install the new file (on miscompare + fix) */
	target_root_install_single_file(file, official_manifest);

	/* at the end of all this, verify the hash again to judge success */
	if (verify_file(file, fullname)) {
		counts.fixed++;
		print(" -> fixed\n");
	} else {
		counts.not_fixed++;
		print(" -> not fixed\n");
	}
end:
	FREE(fullname);
}

static void deal_with_hash_mismatches(struct manifest *official_manifest, struct list *files_to_verify, bool repair)
{
	struct list *iter;
	int complete = 0;
	int list_length;

	info("%s corrupt files\n", repair ? "Repairing" : "Checking for");

	/* for each expected and present file which hash-mismatches vs
	 * the manifest, replace the file */
	iter = list_head(files_to_verify);
	list_length = list_len(iter);

	while (iter) {
		struct file *file;
		file = iter->data;
		iter = iter->next;
		complete++;

		check_and_fix_one(file, official_manifest, repair);
		progress_report(complete, list_length);
	}
}

static int get_dirfd_path(const char *fullname)
{
	int ret = -1;
	int fd;
	char *dir = NULL;
	char *real_path = NULL;

	dir = sys_dirname(fullname);

	fd = open(dir, O_RDONLY);
	if (fd < 0) {
		error("Failed to open dir %s (%s)\n", dir, strerror(errno));
		goto out;
	}

	real_path = realpath(dir, NULL);
	if (!real_path) {
		error("Failed to get real path of %s (%s)\n", dir, strerror(errno));
		close(fd);
		goto out;
	}

	if (str_cmp(real_path, dir) != 0) {
		/* The path to the file contains a symlink, skip the file,
		 * because we cannot safely determine if it can be deleted. */
		ret = -2;
		close(fd);
	} else {
		ret = fd;
	}

	FREE(real_path);
out:
	FREE(dir);
	return ret;
}

static void remove_orphaned_files(struct list *files_to_verify, bool repair)
{
	int ret;
	struct list *iter;
	int list_length = list_len(files_to_verify);
	int complete = 0;

	info("%s extraneous files\n", repair ? "Removing" : "Checking for");

	files_to_verify = list_sort(files_to_verify, cmp_file_filename_reverse);

	iter = list_head(files_to_verify);
	while (iter) {
		struct file *file;
		char *fullname;
		char *base;
		int fd;

		file = iter->data;
		iter = iter->next;
		complete++;

		/*
		 * Don't remove files set as do_not_update, like:
		 *  - Config files
		 *  - ghosted files
		 *  - boot files, that should be managed by the external tool
		 *    clr-boot-manager
		 */
		if (!file->is_deleted || file->do_not_update) {
			goto progress;
		}

		fullname = sys_path_join("%s/%s", globals.path_prefix, file->filename);

		if (!sys_file_exists(fullname)) {
			/* correctly, the file is not present */
			goto out;
		}

		fd = get_dirfd_path(fullname);
		if (fd < 0) {
			if (fd == -1) {
				counts.extraneous++;
				counts.not_deleted++;
			}
			goto out;
		}

		counts.extraneous++;
		info(" -> File that should be deleted: ");
		print("%s%s", fullname, repair ? "" : "\n");

		/* if not repairing, we're done */
		if (!repair) {
			close(fd);
			goto out;
		}

		base = sys_basename(fullname);

		if (!sys_is_dir(fullname)) {
			ret = unlinkat(fd, base, 0);
			if (ret && errno != ENOENT) {
				print(" -> not deleted (%s)\n", strerror(errno));
				counts.not_deleted++;
			} else {
				print(" -> deleted\n");
				counts.deleted++;
			}
		} else {
			ret = unlinkat(fd, base, AT_REMOVEDIR);
			if (ret) {
				counts.not_deleted++;
				if (errno != ENOTEMPTY) {
					print(" -> not deleted (%s)\n", strerror(errno));
				} else {
					//FIXME: Add force removal option?
					print(" -> not deleted (not empty)\n");
				}
			} else {
				print(" -> deleted\n");
				counts.deleted++;
			}
		}
		close(fd);
	out:
		FREE(fullname);
	progress:
		progress_report(complete, list_length);
	}
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
	case 'V':
		if (str_cmp("latest", optarg) == 0) {
			version = -1;
			return true;
		}

		err = str_to_int(optarg, &version);
		if (err < 0 || version < 0) {
			error("Invalid --%s argument: %s\n\n", opt == 'V' ? "version" : "manifest", optarg);
			return false;
		}
		return true;
	case 'f':
		if (!cmdline_command_verify) {
			info("diagnose: unrecognized option '--fix'\n");
			return false;
		}
		warn("\nThe --fix option has been superseded\n");
		info("Please consider using \"swupd repair\" instead\n\n");
		warning_printed = true;
		cmdline_option_fix = true;
		return true;
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
		return true;
	case 'i':
		if (!cmdline_command_verify) {
			info("diagnose: unrecognized option '--install'\n");
			return false;
		}
		warn("\nThe --install option has been superseded\n");
		info("Please consider using \"swupd os-install\" instead\n\n");
		warning_printed = true;
		cmdline_option_install = true;
		cmdline_option_quick = true;
		return true;
	case 'q':
		cmdline_option_quick = optarg_to_bool(optarg);
		return true;
	case 'Y':
		cmdline_option_picky = optarg_to_bool(optarg);
		return true;
	case 'X':
		if (optarg[0] != '/') {
			error("--picky-tree must be an absolute path, for example /usr\n\n");
			return false;
		}
		FREE(cmdline_option_picky_tree);
		cmdline_option_picky_tree = strdup_or_die(optarg);
		return true;
	case 'w':
		cmdline_option_picky_whitelist = strdup_or_die(optarg);
		return true;
	case 'B':
		/* if we are parsing a list from the command line we don't want to append it to
		 * a possible existing list parsed from a config file, we want to replace it, so
		 * we need to delete the existing list first */
		list_free_list(cmdline_option_bundles);
		cmdline_option_bundles = str_split(",", optarg);
		if (!cmdline_option_bundles) {
			error("Missing required --bundles argument\n\n");
			return false;
		}
		return true;
	case FLAG_EXTRA_FILES_ONLY:
		cmdline_option_extra_files_only = optarg_to_bool(optarg);
		return true;
	case FLAG_FILE:
		cmdline_option_file = sys_path_join("%s", optarg);
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
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc > ind) {
		error("unexpected arguments\n\n");
		return false;
	}

	/* warning for "verify" command */
	if (cmdline_command_verify && !warning_printed) {
		warn("\nThe \"swupd verify\" command has been superseded\n");
		info("Please consider using \"swupd diagnose\" instead\n\n");
	}

	/* flag restrictions for "verify" */
	if (cmdline_command_verify) {

		/* flag restrictions for "verify --install" */
		if (cmdline_option_install) {
			if (version == 0) {
				error("--install option requires the --manifest option\n");
				return false;
			}
			if (globals.path_prefix == NULL) {
				error("--install option requires the --path option\n");
				return false;
			}
			if (cmdline_option_fix) {
				error("--install and --fix options are mutually exclusive\n");
				return false;
			}
			if (cmdline_option_picky) {
				error("--install and --picky options are mutually exclusive\n");
				return false;
			}
			if (cmdline_option_extra_files_only) {
				error("--install and --extra-files-only options are mutually exclusive\n");
				return false;
			}
		} else {
			if (version == -1) {
				error("'--manifest latest' only supported with the --install option\n");
				return false;
			}
		}

		if (cmdline_option_bundles) {
			if (cmdline_option_picky || cmdline_option_extra_files_only) {
				warn("Using the --bundles option with --%s may have some undesired side effects\n", cmdline_option_picky ? "picky" : "extra-files-only");
				info("verify will remove all files in %s that are not part of --bundles unless listed in the --picky-whitelist\n", cmdline_option_picky_tree);
			}
			info("\n");
		}

		/* new options should not be allowed for the legacy verify command
		 * since it has been superseded, to motivate users to use diagnose/repair */
		if (cmdline_option_file) {
			error("unrecognized option '--file'\n");
			return false;
		}

	} else {

		/* flag restrictions for "diagnose" */
		if (version == -1) {
			error("'latest' not supported for --version\n");
			return false;
		}

		if (cmdline_option_bundles) {
			if (cmdline_option_picky) {
				error("--bundles and --picky options are mutually exclusive\n");
				return false;
			}
			if (cmdline_option_extra_files_only) {
				error("--bundles and --extra-files-only options are mutually exclusive\n");
				return false;
			}
		}
	}

	/* flag restrictions for both, "verify" and "diagnose" */
	if (cmdline_option_quick) {
		if (cmdline_option_picky) {
			error("--quick and --picky options are mutually exclusive\n");
			return false;
		}
		if (cmdline_option_extra_files_only) {
			error("--quick and --extra-files-only options are mutually exclusive\n");
			return false;
		}
	}
	if (cmdline_option_extra_files_only) {
		if (cmdline_option_picky) {
			error("--extra-files-only and --picky options are mutually exclusive\n");
			return false;
		}
	}

	picky_whitelist = compile_whitelist(cmdline_option_picky_whitelist);
	if (!picky_whitelist) {
		return false;
	}

	return true;
}

static enum swupd_code deal_with_extra_files(struct manifest *manifest, bool fix)
{
	enum swupd_code ret;

	char *start = sys_path_join("%s/%s", globals.path_prefix, cmdline_option_picky_tree);
	info("%s extra files under %s\n", fix ? "Removing" : "Checking for", start);
	ret = walk_tree(manifest, start, fix, picky_whitelist, &counts);
	FREE(start);
	info("\n");

	return ret;
}

static int filter_file_unsafe_to_delete(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;

	A = (struct file *)a;
	B = (struct file *)b;

	ret = str_cmp(A->filename, B->filename);
	if (ret) {
		return ret;
	}

	if (A->is_deleted && !B->is_deleted) {
		/* we found a match*/
		return 0;
	}

	/* both elements in the lists are the same
	 * but not ones we are interested in, move
	 * either list to the next element */
	return -1;
}

static struct list *keep_matching_path(struct list *all_files)
{
	struct list *matching_files = NULL;
	struct list *iter = NULL;
	struct file *file;
	int size;

	for (iter = all_files; iter; iter = iter->next) {
		file = iter->data;

		size = str_len(cmdline_option_file);
		if (strncmp(cmdline_option_file, file->filename, size) == 0) {
			/* if the item entered by the user is a file, the match should
			 * be exact, so look for an exact match first, also look for matches
			 * considering the item as directory */
			if (file->filename[size] == '\0' || file->filename[size] == '/') {
				matching_files = list_append_data(matching_files, file);
			}
		}
	}

	return list_head(matching_files);
}

static enum swupd_code apply_heuristics_for_files(struct list *files)
{
	timelist_timer_start(globals.global_times, "Applying heuristics");
	heuristics_apply(files);
	timelist_timer_stop(globals.global_times);

	return SWUPD_OK;
}

/* This function does a simple verification of files listed in the
 * subscribed bundle manifests.  If the optional "fix" or "install" parameter
 * is specified, the disk will be modified at each point during the
 * sequential comparison of manifest files to disk files, where the disk is
 * found to not match the manifest.  This is notably different from update,
 * which attempts to atomically (or nearly atomically) activate a set of
 * pre-computed and validated staged changes as a group. */
enum swupd_code execute_verify_extra(extra_proc_fn_t post_verify_fn)
{
	struct manifest *official_manifest = NULL;
	int ret;
	struct list *all_subs = NULL;
	struct list *bundles_subs = NULL;
	struct list *selected_subs = NULL;
	struct list *all_submanifests = NULL;
	struct list *bundles_submanifests = NULL;
	struct list *all_files = NULL;
	struct list *bundles_files = NULL;
	struct list *files_to_verify = NULL;
	struct list *iter;
	bool use_latest = false;
	bool invalid_bundle = false;

	if (cmdline_option_statedir_cache != NULL) {
		ret = set_state_dir_cache(cmdline_option_statedir_cache);
		if (ret != true) {
			error("Failed to set statedir-cache\n");
			goto clean_args_and_exit;
		}
	}

	/* Unless we are installing a new bundle and the --skip-optional flag is not
	* set we shoudn't include optional bundles to the bundle list */
	if (!cmdline_option_install || cmdline_option_skip_optional) {
		globals.skip_optional_bundles = true;
	}

	/* Get the current system version and the version to verify against */
	timelist_timer_start(globals.global_times, "Get versions");
	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	int sys_version = get_current_version(globals.path_prefix);
	if (!version) {
		if (sys_version < 0) {
			error("Unable to determine current OS version\n");
			ret = SWUPD_CURRENT_VERSION_UNKNOWN;
			goto clean_and_exit;
		}
		version = sys_version;
	}

	/* If "latest" was chosen, get latest version */
	if (version == -1) {
		use_latest = true;
		version = get_latest_version(NULL);
		if (version < 0) {
			error("Unable to get latest version for install\n");
			ret = SWUPD_SERVER_CONNECTION_ERROR;
			goto clean_and_exit;
		}
	}
	timelist_timer_stop(globals.global_times); // closing: Get versions

	if (cmdline_option_install) {
		info("Installing OS version %i%s\n", version, use_latest ? " (latest)" : "");
	} else {
		info("%s version %i\n", cmdline_command_verify ? "Verifying" : "Diagnosing", version);
	}

	/*
	 * FIXME: We need a command line option to override this in case the
	 * certificate is hosed and the admin knows it and wants to recover.
	 */
	timelist_timer_start(globals.global_times, "Clean up download directory");
	ret = rm_staging_dir_contents("download");
	if (ret != 0) {
		warn("Failed to remove prior downloads, carrying on anyway\n");
	}
	timelist_timer_stop(globals.global_times); // closing: Clean up download directory

	timelist_timer_start(globals.global_times, "Load manifests");

	/* Gather current manifests */
	official_manifest = load_mom(version, NULL);
	if (!official_manifest) {
		/* This is hit when or if an OS version is specified for --fix which
		 * is not available, or if there is a server error and a manifest is
		 * not provided.
		 */
		error("Unable to download/verify %d Manifest.MoM\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;

		/* No repair is possible without a manifest, nor is accurate reporting
		 * of the state of the system. Therefore cleanup, report failure and exit
		 */
		goto clean_and_exit;
	}

	if (!is_compatible_format(official_manifest->manifest_version)) {
		if (cmdline_option_force) {
			warn("The --force option is specified; ignoring"
			     " format mismatch for diagnose\n");
		} else {
			error("Mismatching formats detected when %s %d"
			      " (expected: %s; actual: %d)\n",
			      cmdline_command_verify ? "verifying" : "diagnosing",
			      version, globals.format_string, official_manifest->manifest_version);
			int latest = get_latest_version(NULL);
			if (latest > 0) {
				info("Latest supported version to %s: %d\n",
				     cmdline_command_verify ? "verify" : "diagnose", latest);
			}
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto clean_and_exit;
		}
	}

	/* If --fix was specified but --force and --picky are not present, check
	 * that the user is only fixing to their current version. If they are
	 * fixing to a different version, print an error message that they need to
	 * specify --force or --picky */
	if (cmdline_option_fix && !is_current_version(version)) {
		// TODO(castulo): we should evaluate if --force should be used for --picky too
		if (cmdline_option_force || cmdline_option_picky) {
			warn("The --%s option is specified; ignoring version mismatch "
			     "for repair\n",
			     cmdline_option_force ? "force" : "picky");
		} else {
			error("Repairing to a different version requires "
			      "--force or --picky\n");
			ret = SWUPD_INVALID_OPTION;
			goto clean_and_exit;
		}
	}

	/* get the list of subscribed (installed) bundles */
	read_subscriptions(&all_subs);
	selected_subs = all_subs;
	if (cmdline_option_bundles) {
		info("\n");
		for (iter = list_head(cmdline_option_bundles); iter; iter = iter->next) {
			/* let's validate the bundle names provided by the user first */
			struct file *file;
			file = mom_search_bundle(official_manifest, iter->data);
			if (!file) {
				/* bundle is invalid or no longer available, nothing to do with
				 * this one for now */
				invalid_bundle = true;
				warn("Bundle \"%s\" is invalid or no longer available\n\n", (char *)iter->data);
				continue;
			}
			if (!component_subscribed(all_subs, iter->data)) {
				/* the requested bundle is not installed, there are two use cases
				 * where we allow users to specify a bundle that is not installed:
				 * - when using "verify --fix --bundles" (legacy way to install a bundle)
				 * - when using "os-install --bundles" or "verify --install --bundles"
				 * In these cases we need to make sure the subscriptions in bundles_subs
				 * are also in the all_subs list or they will just be ignored */
				if ((cmdline_command_verify && cmdline_option_fix) || (cmdline_option_install)) {
					create_and_append_subscription(&all_subs, iter->data);
				} else {
					warn("Bundle \"%s\" is not installed, skipping it...\n\n", (char *)iter->data);
					continue;
				}
			}
			create_and_append_subscription(&bundles_subs, iter->data);
		}
		selected_subs = bundles_subs;
		/* the user should have specified at least one valid bundle, otherwise finish */
		if (!bundles_subs) {
			info("No valid bundles were specified, nothing to be done\n");
			goto clean_and_exit;
		}
	}

	if (cmdline_option_bundles && !cmdline_option_install) {
		info("Limiting %s to the following bundles:\n", cmdline_command_verify ? "verify" : "diagnose");
		for (iter = list_head(bundles_subs); iter; iter = iter->next) {
			struct sub *sub = iter->data;
			info(" - %s\n", sub->component);
		}
		info("\n");
	}

	info("Downloading missing manifests...\n");
	ret = add_included_manifests(official_manifest, &all_subs);
	if (cmdline_option_bundles && !(ret & add_sub_ERR)) {
		ret = add_included_manifests(official_manifest, &bundles_subs);
	}
	/* if one or more of the installed bundles were not found in the manifest,
	 * continue only if --force was used since the bundles could be removed */
	if (ret || invalid_bundle) {
		if ((ret & add_sub_BADNAME) || invalid_bundle) {
			if (cmdline_option_force) {
				if (cmdline_option_picky && cmdline_option_fix) {
					warn("\nOne or more installed bundles that are not "
					     "available at version %d will be removed\n",
					     version);
				} else if (cmdline_option_picky && !cmdline_option_fix) {
					warn("\nOne or more installed bundles are not "
					     "available at version %d\n",
					     version);
				}
				ret = SWUPD_OK;
			} else {
				if (cmdline_option_install || cmdline_option_bundles) {
					error("\nOne or more of the provided bundles are not available at version %d\n", version);
					info("Please make sure the name of the provided bundles are correct, or use --force to override\n")
				} else {
					error("\nUnable to verify. One or more currently installed bundles "
					      "are not available at version %d. Use --force to override\n",
					      version);
				}
				ret = SWUPD_INVALID_BUNDLE;
				goto clean_and_exit;
			}
		} else {
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto clean_and_exit;
		}
	}

	set_subscription_versions(official_manifest, NULL, &all_subs);
	set_subscription_versions(official_manifest, NULL, &bundles_subs);
	all_submanifests = recurse_manifest(official_manifest, all_subs, NULL, false, NULL);
	if (!all_submanifests) {
		error("Cannot load MoM sub-manifests\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto clean_and_exit;
	}
	official_manifest->submanifests = all_submanifests;

	if (cmdline_option_bundles) {
		bundles_submanifests = recurse_manifest(official_manifest, bundles_subs, NULL, false, NULL);
		if (!bundles_submanifests) {
			error("Cannot load MoM sub-manifests\n");
			ret = SWUPD_RECURSE_MANIFEST;
			goto clean_and_exit;
		}
		official_manifest->submanifests = bundles_submanifests;
	}
	timelist_timer_stop(globals.global_times); // closing: Load manifests

	timelist_timer_start(globals.global_times, "Consolidate files from bundles");
	all_files = consolidate_files_from_bundles(all_submanifests);
	official_manifest->files = all_files;
	if (cmdline_option_bundles) {
		bundles_files = consolidate_files_from_bundles(bundles_submanifests);
		/* we need to make sure the files from the selected bundles
		 * are compared against the full list of consolidated files
		 * from all bundles so we don't end up deleting a file that
		 * is needed by another bundle */
		bundles_files = list_sorted_filter_common_elements(bundles_files, all_files, filter_file_unsafe_to_delete, NULL);
		official_manifest->files = bundles_files;

		/* at this point we no longer need the data regarding bundles
		 * not specified by the user with the --bundles flag, we can free it */
		list_free_list(all_files);
		list_free_list_and_data(all_submanifests, manifest_free_data);
	}
	timelist_timer_stop(globals.global_times);

	/* get the list of files to verify */
	files_to_verify = official_manifest->files;
	if (cmdline_option_file) {
		/* the user specified a file or path to verify, use that instead */
		char *file_path = sys_path_join("%s/%s", globals.path_prefix, cmdline_option_file);
		info("\nLimiting diagnose to the following %s:\n", sys_filelink_is_dir(file_path) ? "directory (recursively)" : "file");
		info(" - %s\n", cmdline_option_file);
		FREE(file_path);

		files_to_verify = keep_matching_path(official_manifest->files);
		if (!files_to_verify) {
			/* no tracked files found using the criteria,
			 * just handle extra files */
			goto extra_files;
		}
	}
	apply_heuristics_for_files(files_to_verify);

	if (cmdline_option_extra_files_only) {
		/* user wants to deal only with the extra files, so skip everything else */
		goto extra_files;
	}

	/* steps 5, 6 & 7 are executed within the get_required_files function */
	timelist_timer_start(globals.global_times, "Get required files");

	/* get the initial number of files to be inspected */
	counts.checked = list_len(files_to_verify);

	/* when fixing or installing we need input files */
	if (cmdline_option_fix || cmdline_option_install) {
		ret = get_required_files(official_manifest, files_to_verify, selected_subs);
		if (ret != 0) {
			ret = SWUPD_COULDNT_DOWNLOAD_FILE;
			goto clean_and_exit;
		}
	}

	/* content is not installed when download flag is set */
	if (cmdline_option_download) {
		goto clean_and_exit;
	}

	timelist_timer_stop(globals.global_times);

	/* preparation work complete. */

	/*
	 * NOTHING ELSE IS ALLOWED TO FAIL/ABORT after this line.
	 * This tool is there to recover a nearly-bricked system. Aborting
	 * from this point forward, for any reason, will result in a bricked system.
	 *
	 * I don't care what your static analysis tools says
	 * I don't care what valgrind tells you
	 *
	 * There shall be no "goto fail;" from this point on.
	 *
	 *   *** THE SHOW MUST GO ON ***
	 */

	/*
	 * Next put the files in place that are missing completely.
	 * This is to avoid updating a symlink to a library before the new full file
	 * is already there. It's also the most safe operation, adding files rarely
	 * has unintended side effect. So lets do the safest thing first.
	 */
	timelist_timer_start(globals.global_times, "Add missing files");
	progress_next_step("add_missing_files", PROGRESS_BAR);
	add_missing_files(official_manifest, files_to_verify, cmdline_option_fix || cmdline_option_install);
	timelist_timer_stop(globals.global_times);

	if (cmdline_option_quick) {
		/* quick only replaces missing files, so it is done here */
		goto brick_the_system_and_clean_curl;
	}

	/* repair corrupt files */
	timelist_timer_start(globals.global_times, "Fixing modified files");
	progress_next_step("fix_files", PROGRESS_BAR);
	deal_with_hash_mismatches(official_manifest, files_to_verify, cmdline_option_fix);
	timelist_timer_stop(globals.global_times);

	/* remove orphaned files, removing files could be
	 * risky, so only do it if the prior phases had no problems */
	timelist_timer_start(globals.global_times, "Removing orphaned files");
	if ((counts.not_fixed == 0) && (counts.not_replaced == 0)) {
		progress_next_step("remove_extraneous_files", PROGRESS_BAR);
		remove_orphaned_files(files_to_verify, cmdline_option_fix);
	} else {
		info("The removal of extraneous files will be skipped due to the previous errors found repairing\n");
	}
	timelist_timer_stop(globals.global_times);

	/* remove extra files */
extra_files:
	if (cmdline_option_picky || cmdline_option_extra_files_only) {
		timelist_timer_start(globals.global_times, "Removing extra files");
		progress_next_step("remove_extra_files", PROGRESS_UNDEFINED);
		deal_with_extra_files(official_manifest, cmdline_option_fix);
		timelist_timer_stop(globals.global_times);
	}

brick_the_system_and_clean_curl:
	/* clean up */

	/*
	 * naming convention: All exit goto labels must follow the "brick_the_system_and_FOO:" pattern
	 */

	/* track bundles specified by user */
	if (cmdline_option_install && cmdline_option_bundles) {
		/* this is a fresh install so we need to specify the
		 * state dir of the new install */
		char *new_os_statedir = sys_path_join("%s/%s", globals.path_prefix, "/var/lib/swupd");
		for (iter = cmdline_option_bundles; iter; iter = iter->next) {
			char *bundle = iter->data;
			/* make sure the bundle was in fact valid and will
			 * be installed before creating the tracking file */
			if (list_search(bundles_subs, bundle, cmp_sub_component_string)) {
				track_bundle_in_statedir(bundle, new_os_statedir);
			}
		}
		FREE(new_os_statedir);
	}

	/* report a summary of what we managed to do and not do */
	info("Inspected %i file%s\n", counts.checked, (counts.checked == 1 ? "" : "s"));

	if (counts.missing) {
		info("  %i file%s %s missing\n", counts.missing, (counts.missing > 1 ? "s" : ""), (counts.missing > 1 ? "were" : "was"));
		if (cmdline_option_fix || cmdline_option_install) {
			info("    %i of %i missing files were %s\n", counts.replaced, counts.missing, cmdline_option_install ? "installed" : "replaced");
			info("    %i of %i missing files were not %s\n", counts.not_replaced, counts.missing, cmdline_option_install ? "installed" : "replaced");
		}
	}

	if (counts.mismatch) {
		info("  %i file%s did not match\n", counts.mismatch, (counts.mismatch > 1 ? "s" : ""));
		if (cmdline_option_fix) {
			info("    %i of %i files were repaired\n", counts.fixed, counts.mismatch);
			info("    %i of %i files were not repaired\n", counts.not_fixed, counts.mismatch);
		}
	}

	if (counts.extraneous) {
		info("  %i file%s found which should be deleted\n", counts.extraneous, (counts.extraneous > 1 ? "s" : ""));
		if (cmdline_option_fix) {
			info("    %i of %i files were deleted\n", counts.deleted, counts.extraneous);
			info("    %i of %i files were not deleted\n", counts.not_deleted, counts.extraneous);
		}
	}

	if (cmdline_option_fix || cmdline_option_install) {
		progress_next_step("run_postupdate_scripts", PROGRESS_UNDEFINED);
		// always run in a fix or install case
		globals.need_update_bootmanager = true;
		timelist_timer_start(globals.global_times, "Run Scripts");
		info("\n");
		scripts_run_post_update(globals.wait_for_scripts);
		timelist_timer_stop(globals.global_times);
	}

	sync();

	if ((counts.not_fixed == 0) &&
	    (counts.not_replaced == 0) &&
	    ((counts.not_deleted == 0) ||
	     ((counts.not_deleted != 0) && !cmdline_option_fix)) &&
	    !ret) {
		ret = SWUPD_OK;
	} else {
		/* If something failed to be fixed/replaced/deleted but the ret value
		 * is zero then use a generic error message for verify, use the actual
		 * ret value otherwise */
		if (!ret) {
			ret = SWUPD_VERIFY_FAILED;
		}
	}

	/* this concludes the critical section, after this point it's clean up time, the disk content is finished and final */

	/* execute post-verify processing (if any) */
	if (post_verify_fn) {
		ret = post_verify_fn(files_to_verify);
	}

clean_and_exit:
	if (cmdline_option_file) {
		list_free_list(files_to_verify);
	}
	free_subscriptions(&all_subs);
	free_subscriptions(&bundles_subs);
	/* if the --bundles flag was used the official_manifest contains
	 * bundles_files and bundles_submanifests, so they will be freed
	 * along with the official_manifest */
	manifest_free(official_manifest);

	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_LOW,
		  "verify",
		  "fix=%d\nret=%d\n"
		  "current_version=%d\n"
		  "file_replaced_count=%d\n"
		  "file_not_replaced_count=%d\n"
		  "file_missing_count=%d\n"
		  "file_fixed_count=%d\n"
		  "file_not_fixed_count=%d\n"
		  "file_deleted_count=%d\n"
		  "file_not_deleted_count=%d\n"
		  "file_mismatch_count=%d\n"
		  "file_extraneous_count=%d\n"
		  "bytes=%ld\n",
		  cmdline_option_fix || cmdline_option_install,
		  ret,
		  version,
		  counts.replaced,
		  counts.not_replaced,
		  counts.missing,
		  counts.fixed,
		  counts.not_fixed,
		  counts.deleted,
		  counts.not_deleted,
		  counts.mismatch,
		  counts.extraneous,
		  total_curl_sz);

	/* suggestion to fix problems */
	if (!cmdline_option_install && !cmdline_option_fix && (counts.mismatch > 0 || counts.missing > 0 || counts.extraneous > 0)) {
		info("\nUse \"swupd repair%s\" to correct the problems in the system\n", counts.picky_extraneous > 0 ? " --picky" : "");
	}

	if (ret == SWUPD_OK) {
		if (cmdline_option_download) {
			info("\nInstallation files downloaded\n");
		} else if (cmdline_option_install) {
			info("\nInstallation successful\n");

			if (counts.not_replaced > 0) {
				ret = SWUPD_NO;
			}
		} else if (cmdline_option_fix) {
			info("\nRepair successful\n");

			if (counts.not_fixed > 0 ||
			    counts.not_replaced > 0 ||
			    counts.not_deleted > 0) {
				ret = SWUPD_NO;
			}
		} else {
			/* This is just a verification */
			info("\n%s successful\n", cmdline_command_verify ? "Verify" : "Diagnose");

			if (counts.mismatch > 0 ||
			    counts.missing > 0 ||
			    counts.extraneous > 0) {
				ret = SWUPD_NO;
			}
		}
	} else {
		if (cmdline_option_fix) {
			info("\nRepair did not fully succeed\n");
		} else if (cmdline_option_download) {
			info("\nInstallation file downloads failed\n");
		} else if (cmdline_option_install) {
			info("\nInstallation failed\n");
		} else {
			/* This is just a verification */
			info("\n%s did not fully succeed\n", cmdline_command_verify ? "Verify" : "Diagnose");
		}
	}

	timelist_print_stats(globals.global_times);

clean_args_and_exit:
	list_free_list_and_data(cmdline_option_bundles, free);

	/* rest counters */
	counts = (struct file_counts){ 0 };

	return ret;
}

enum swupd_code execute_verify(void)
{
	return execute_verify_extra(NULL);
}

enum swupd_code verify_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_verify = 12;
	string_or_die(&cmdline_option_picky_tree, "%s", picky_tree_default);

	/* set option needed so we know the legacy "verify" command was used */
	verify_set_command_verify(true);

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		FREE(cmdline_option_picky_tree);
		return ret;
	}

	/*
	 * Steps for verify:
	 *  1) load_manifests
	 *  2) download_packs
	 *  3) extract_packs
	 *  4) check_files_hash
	 *  5) validate_fullfiles
	 *  6) download_fullfiles
	 *  7) extract_fullfiles
	 *  8) add_missing_files
	 *  9) fix_files
	 *  10) remove_extraneous_files
	 *  11) remove_extra_files
	 *  12) run_postupdate_scripts
	 *
	 *  TODO(castulo): steps in verify have to many variables so it is
	 *  being left as constant for know, this should not be much of a
	 *  problem since it is a superseded command.
	 */
	progress_init_steps("verify", steps_in_verify);

	/* diagnose */
	ret = execute_verify();

	FREE(cmdline_option_picky_tree);
	if (picky_whitelist) {
		regfree(picky_whitelist);
		picky_whitelist = NULL;
	}
	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}

enum swupd_code diagnose_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	int steps_in_diagnose;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret;
	}

	/* if the --file flag was used, use that path for --picky as well
	 * unless --picky-tree was also specified, if none use default */
	if (!cmdline_option_picky_tree && !cmdline_option_file) {
		string_or_die(&cmdline_option_picky_tree, "%s", picky_tree_default);
	} else if (!cmdline_option_picky_tree) {
		cmdline_option_picky_tree = cmdline_option_file;
	}

	/*
	 * Steps for diagnose:
	 *  1) load_manifests (with --extra-files-only jumps to step 5)
	 *  2) add_missing_files (finishes here on --quick)
	 *  3) fix_files
	 *  4) remove_extraneous_files
	 *  5) remove_extra_files (only with --picky or with --extra-files-only)
	 */
	if (cmdline_option_extra_files_only || cmdline_option_quick) {
		steps_in_diagnose = 2;
	} else if (cmdline_option_picky) {
		steps_in_diagnose = 5;
	} else {
		steps_in_diagnose = 4;
	}
	progress_init_steps("diagnose", steps_in_diagnose);

	/* diagnose */
	ret = execute_verify();

	if (cmdline_option_picky_tree != cmdline_option_file) {
		FREE(cmdline_option_file);
	}
	FREE(cmdline_option_picky_tree);
	if (picky_whitelist) {
		regfree(picky_whitelist);
		picky_whitelist = NULL;
	}
	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
