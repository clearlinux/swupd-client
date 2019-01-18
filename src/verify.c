/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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

#define _GNU_SOURCE
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

#include "config.h"
#include "signature.h"
#include "swupd.h"

static const char picky_whitelist_default[] = "/usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src";

static bool cmdline_option_fix = false;
static bool cmdline_option_picky = false;
static const char *cmdline_option_picky_tree = "/usr";
static const char *cmdline_option_picky_whitelist = picky_whitelist_default;
static bool cmdline_option_install = false;
static bool cmdline_option_quick = false;
static struct list *cmdline_bundles = NULL;

/* picky_whitelist points to picky_whitelist_buffer if and only if regcomp() was called for it */
static regex_t *picky_whitelist;
static regex_t picky_whitelist_buffer;

static int version = 0;

/* Count of how many files we managed to not fix */
static struct file_counts counts;

static const struct option prog_opts[] = {
	{ "fix", no_argument, 0, 'f' },
	{ "force", no_argument, 0, 'x' },
	{ "install", no_argument, 0, 'i' },
	{ "manifest", required_argument, 0, 'm' },
	{ "picky", no_argument, 0, 'Y' },
	{ "picky-tree", required_argument, 0, 'X' },
	{ "picky-whitelist", required_argument, 0, 'w' },
	{ "quick", no_argument, 0, 'q' },
	{ "bundles", required_argument, 0, 'B' },
};

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd verify [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -m, --manifest=M        Verify against manifest version M\n");
	fprintf(stderr, "   -f, --fix               Fix local issues relative to server manifest (will not modify ignored files)\n");
	fprintf(stderr, "   -Y, --picky             List (without --fix) or remove (with --fix) files which should not exist\n");
	fprintf(stderr, "   -X, --picky-tree=[PATH] Selects the sub-tree where --picky looks for extra files. Default: /usr\n");
	fprintf(stderr, "   -w, --picky-whitelist=[RE] Any path completely matching the POSIX extended regular expression is ignored by --picky. Matched directories get skipped. Example: /var|/etc/machine-id. Default: %s\n", picky_whitelist_default);
	fprintf(stderr, "   -i, --install           Similar to \"--fix\" but optimized for install all files to empty directory\n");
	fprintf(stderr, "   -q, --quick             Don't compare hashes, only fix missing files\n");
	fprintf(stderr, "   -B, --bundles=[BUNDLES] Ensure BUNDLES are installed correctly. Example: --bundles=os-core,vi\n");
	fprintf(stderr, "\n");
}

static bool compile_whitelist()
{
	int errcode;
	char *error_buffer = NULL;
	char *full_regex = NULL;
	bool success = false;

	/* Enforce matching the entire path. */
	string_or_die(&full_regex, "^(%s)$", cmdline_option_picky_whitelist);

	assert(!picky_whitelist);
	errcode = regcomp(&picky_whitelist_buffer, full_regex, REG_NOSUB | REG_EXTENDED);
	picky_whitelist = &picky_whitelist_buffer;
	if (errcode) {
		size_t len;
		len = regerror(errcode, picky_whitelist, NULL, 0);
		error_buffer = malloc(len);
		ON_NULL_ABORT(error_buffer);

		regerror(errcode, picky_whitelist, error_buffer, len);
		fprintf(stderr, "Invalid --picky-whitelist=%s: %s\n", cmdline_option_picky_whitelist, error_buffer);
		goto done;
	}
	success = true;

done:
	free_string(&full_regex);
	if (error_buffer) {
		free_string(&error_buffer);
	}
	return success;
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
	ret = download_subscribed_packs(subs, official_manifest);
	if (ret < 0) { // require zero pack
		/* If we hit this point, we know we have a network connection, therefore
		 * 	the error is server-side. This is also a critical error, so detailed
		 * 	logging needed */
		fprintf(stderr, "zero pack downloads failed\n");
		fprintf(stderr, "Failed - Server-side error, cannot download necessary files\n");
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
	unsigned int complete = 0;
	unsigned int total = list_len(files);
	int ret = 1;

	fprintf(stderr, "Verifying files\n");
	iter = list_head(files);
	while (iter) {
		struct file *f = iter->data;
		char *fullname;
		bool valid;

		print_progress(complete, total);
		complete++;
		iter = iter->next;
		if (f->is_deleted || f->do_not_update) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, f->filename);
		valid = cmdline_option_quick ? verify_file_lazy(fullname) : verify_file(f, fullname);
		free_string(&fullname);
		if (valid) {
			f->do_not_update = 1;
		} else {
			ret = 0;
		}
	}
	print_progress(total, total);
	printf("\n");

	return ret;
}

/* allow optimization of install case */
static int get_required_files(struct manifest *official_manifest, struct list *subs)
{
	int ret;

	if (cmdline_option_install) {
		get_all_files(official_manifest, subs);
	}

	if (check_files_hash(official_manifest->files)) {
		return 0;
	}

	ret = download_fullfiles(official_manifest->files, NULL);
	if (ret) {
		fprintf(stderr, "Error: Unable to download necessary files for this OS release\n");
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
		fprintf(stderr, "\tWarning: No space left on device\n");
		goto out;
	}

	string_or_die(&original, "%s/staged/%s", state_dir, file->hash);
	fs_free = get_available_space(path_prefix);
	if (fs_free < 0 || stat(original, &st) != 0) {
		fprintf(stderr, "\tWarning: Unable to determine free space on filesystem.\n");
		goto out;
	}

	if (fs_free < st.st_size * 1.1) {
		fprintf(stderr, "\tWarning: File to install (%s) too large by %ldK.\n",
			file->filename, (st.st_size - fs_free) / 1000);
		/* set flag to skip checking space on the second failure, assume we're still out of space */
		no_freespace_flag = true;
	}

out:
	fprintf(stderr, "\tContinuing operation...\n");
	free_string(&original);
}

/* for each missing but expected file, (re)add the file */
static void add_missing_files(struct manifest *official_manifest, bool repair)
{
	int ret;
	struct file local;
	struct list *iter;
	unsigned int list_length = list_len(official_manifest->files);
	unsigned int complete = 0;

	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;
		char *fullname;

		file = iter->data;
		iter = iter->next;
		complete++;

		if ((file->is_deleted) ||
		    (file->do_not_update)) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, file->filename);
		memset(&local, 0, sizeof(struct file));
		local.filename = file->filename;
		populate_file_struct(&local, fullname);
		ret = compute_hash_lazy(&local, fullname);
		if (ret != 0) {
			counts.not_replaced++;
			free_string(&fullname);
			continue;
		}

		/* compare the hash and report mismatch */
		if (hash_is_zeros(local.hash)) {
			counts.missing++;
			if (!repair || (repair && cmdline_option_install == false)) {
				/* Log to stdout, so we can post-process */
				printf("\nMissing file: %s\n", fullname);
			}
		} else {
			free_string(&fullname);
			continue;
		}

		/* if not repairing, we're done */
		if (!repair) {
			goto out;
		}

		/* install the new file (on miscompare + fix) */
		ret = do_staging(file, official_manifest);
		if (ret == 0) {
			rename_staged_file_to_final(file);
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
			printf("\n\tnot fixed\n");

			check_warn_freespace(file);

		} else {
			counts.replaced++;
			file->do_not_update = 1;
			if (cmdline_option_install == false) {
				printf("\n\tfixed\n");
			}
		}
	out:
		free_string(&fullname);
		print_progress(complete, list_length);
	}
	print_progress(list_length, list_length);
	printf("\n");
}

static void check_and_fix_one(struct file *file, struct manifest *official_manifest, bool repair)
{
	char *fullname;
	int ret;

	// Note: boot files not marked as deleted are candidates for verify/fix
	if (file->is_deleted || ignore(file) || file->do_not_update) {
		return;
	}

	/* compare the hash and report mismatch */
	fullname = mk_full_filename(path_prefix, file->filename);
	if (verify_file(file, fullname)) {
		goto end;
	}
	// do not account for missing files at this point, they are
	// accounted for in a different stage, only account for mismatch
	if (access(fullname, F_OK) == 0) {
		counts.mismatch++;
		/* Log to stdout, so we can post-process it */
		printf("\nHash mismatch for file: %s\n", fullname);
	}

	/* if not repairing, we're done */
	if (!repair) {
		goto end;
	}

	/* install the new file (on miscompare + fix) */
	ret = do_staging(file, official_manifest);
	if (ret == 0) {
		rename_staged_file_to_final(file);
	}

	/* at the end of all this, verify the hash again to judge success */
	if (verify_file(file, fullname)) {
		counts.fixed++;
		printf("\tfixed\n");
	} else {
		counts.not_fixed++;
		printf("\tnot fixed\n");
	}
end:
	free_string(&fullname);
}

static void deal_with_hash_mismatches(struct manifest *official_manifest, bool repair)
{
	struct list *iter;
	int complete = 0;
	int list_length;

	/* for each expected and present file which hash-mismatches vs
	 * the manifest, replace the file */
	iter = list_head(official_manifest->files);
	list_length = list_len(iter);

	while (iter) {
		struct file *file;
		file = iter->data;
		iter = iter->next;
		complete++;

		check_and_fix_one(file, official_manifest, repair);
		print_progress(complete, list_length);
	}
	printf("\n"); /* Finish update progress message */
}

static void remove_orphaned_files(struct manifest *official_manifest, bool repair)
{
	int ret;
	struct list *iter;

	official_manifest->files = list_sort(official_manifest->files, file_sort_filename_reverse);

	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;
		char *fullname;
		char *base;
		struct stat sb;
		int fd;

		file = iter->data;
		iter = iter->next;

		/* Do not remove files that have not been deleted, are config files, or
		 * are marked as ghosted */
		if (!file->is_deleted || file->is_config || file->is_ghosted) {
			continue;
		}

		/* Note: boot files marked as deleted should not be deleted by
		 * verify/fix; this task is delegated to an external program
		 * (currently /usr/bin/clr-boot-manager).
		 */
		if (ignore(file)) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, file->filename);

		if (lstat(fullname, &sb) != 0) {
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
		printf("File that should be deleted: %s\n", fullname);

		/* if not repairing, we're done */
		if (!repair) {
			close(fd);
			goto out;
		}

		base = basename(fullname);

		if (!S_ISDIR(sb.st_mode)) {
			ret = unlinkat(fd, base, 0);
			if (ret && errno != ENOENT) {
				fprintf(stderr, "Failed to remove %s (%i: %s)\n", fullname, errno, strerror(errno));
				counts.not_deleted++;
			} else {
				fprintf(stderr, "\tdeleted\n");
				counts.deleted++;
			}
		} else {
			ret = unlinkat(fd, base, AT_REMOVEDIR);
			if (ret) {
				counts.not_deleted++;
				if (errno != ENOTEMPTY) {
					fprintf(stderr, "Failed to remove empty folder %s (%i: %s)\n",
						fullname, errno, strerror(errno));
				} else {
					//FIXME: Add force removal option?
					fprintf(stderr, "Couldn't remove directory containing untracked files: %s\n", fullname);
				}
			} else {
				fprintf(stderr, "\tdeleted\n");
				counts.deleted++;
			}
		}
		close(fd);
	out:
		free_string(&fullname);
	}
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
		if (strcmp("latest", optarg) == 0) {
			version = -1;
			return true;
		}

		err = strtoi_err(optarg, &version);
		if (err < 0 || version < 0) {
			fprintf(stderr, "Invalid --manifest argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'f':
		cmdline_option_fix = true;
		return true;
	case 'x':
		force = true;
		return true;
	case 'i':
		cmdline_option_install = true;
		cmdline_option_quick = true;
		return true;
	case 'q':
		cmdline_option_quick = true;
		return true;
	case 'Y':
		cmdline_option_picky = true;
		return true;
	case 'X':
		if (optarg[0] != '/') {
			fprintf(stderr, "--picky-tree must be an absolute path, for example /usr\n\n");
			return false;
		}
		cmdline_option_picky_tree = optarg;
		return true;
	case 'w':
		cmdline_option_picky_whitelist = optarg;
		return true;
	case 'B': {
		char *arg_copy = strdup_or_die(optarg);
		char *token = strtok(arg_copy, ",");
		while (token) {
			cmdline_bundles = list_prepend_data(cmdline_bundles, strdup_or_die(token));
			token = strtok(NULL, ",");
		}
		free(arg_copy);
		if (!cmdline_bundles) {
			fprintf(stderr, "Missing --bundle argument\n\n");
			return false;
		}
		return true;
	}
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
		fprintf(stderr, "Error: unexpected arguments\n\n");
		return false;
	}

	if (cmdline_option_install) {
		if (version == 0) {
			fprintf(stderr, "--install option requires -m version option\n");
			return false;
		}
		if (path_prefix == NULL) {
			fprintf(stderr, "--install option requires --path option\n");
			return false;
		}
		if (cmdline_option_fix) {
			fprintf(stderr, "--install and --fix options are mutually exclusive\n");
			return false;
		}
	} else if (version == -1) {
		fprintf(stderr, "-m latest only supported with --install\n");
		return false;
	}
	if (!compile_whitelist()) {
		return false;
	}

	return true;
}

/* This function does a simple verification of files listed in the
 * subscribed bundle manifests.  If the optional "fix" or "install" parameter
 * is specified, the disk will be modified at each point during the
 * sequential comparison of manifest files to disk files, where the disk is
 * found to not match the manifest.  This is notably different from update,
 * which attempts to atomically (or nearly atomically) activate a set of
 * pre-computed and validated staged changes as a group. */
enum swupd_code verify_main(int argc, char **argv)
{
	struct manifest *official_manifest = NULL;
	int ret;
	int retries = 0;
	int timeout = 10;
	struct list *subs = NULL;

	if (!parse_options(argc, argv)) {
		ret = SWUPD_INVALID_OPTION;
		print_help();
		goto clean_args_and_exit;
	}

	/* parse command line options */
	assert(argc >= 0);
	assert(argv != NULL);

	ret = swupd_init();
	if (ret != 0) {
		fprintf(stderr, "Failed verify initialization, exiting now.\n");
		goto clean_args_and_exit;
	}

	/* Get the current system version and the version to verify against */
	int sys_version = get_current_version(path_prefix);
	if (!version) {
		if (sys_version < 0) {
			fprintf(stderr, "Error: Unable to determine current OS version\n");
			ret = SWUPD_CURRENT_VERSION_UNKNOWN;
			goto clean_and_exit;
		}
		version = sys_version;
	}

	/* If "latest" was chosen, get latest version */
	if (version == -1) {
		version = get_latest_version(NULL);
		if (version < 0) {
			fprintf(stderr, "Unable to get latest version for install\n");
			ret = SWUPD_SERVER_CONNECTION_ERROR;
			goto clean_and_exit;
		}
	}

	fprintf(stderr, "Verifying version %i\n", version);

	if (cmdline_bundles) {
		while (cmdline_bundles) {
			create_and_append_subscription(&subs, cmdline_bundles->data);
			cmdline_bundles = list_free_item(cmdline_bundles, free);
		}
	} else {
		read_subscriptions(&subs);
	}

	/*
	 * FIXME: We need a command line option to override this in case the
	 * certificate is hosed and the admin knows it and wants to recover.
	 */

	ret = rm_staging_dir_contents("download");
	if (ret != 0) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
		fprintf(stderr, "Failed to remove prior downloads, carrying on anyway\n");
	}

	timelist_timer_start(global_times, "Load and recurse Manifests");

	/* Gather current manifests */
	/* When the version we are verifying against does not match our system version
	 * disable checks for mixer state so the user can easily switch back to their
	 * normal update stream */
	if (version != sys_version) {
		official_manifest = load_mom(version, false, false, NULL);
	} else {
		official_manifest = load_mom(version, false, system_on_mix(), NULL);
	}

	if (!official_manifest) {
		/* This is hit when or if an OS version is specified for --fix which
		 * is not available, or if there is a server error and a manifest is
		 * not provided.
		 */
		fprintf(stderr, "Unable to download/verify %d Manifest.MoM\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;

		/* No repair is possible without a manifest, nor is accurate reporting
		 * of the state of the system. Therefore cleanup, report failure and exit
		 */
		goto clean_and_exit;
	}

	if (!is_compatible_format(official_manifest->manifest_version)) {
		if (force) {
			fprintf(stderr, "WARNING: the force option is specified; ignoring"
					" format mismatch for verify\n");
		} else {
			fprintf(stderr, "ERROR: Mismatching formats detected when verifying %d"
					" (expected: %s; actual: %d)\n",
				version, format_string, official_manifest->manifest_version);
			int latest = get_latest_version(NULL);
			if (latest > 0) {
				fprintf(stderr, "Latest supported version to verify: %d\n", latest);
			}
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto clean_and_exit;
		}
	}

	/* If --fix was specified but --force and --picky are not present, check
	 * that the user is only fixing to their current version. If they are
	 * fixing to a different version, print an error message that they need to
	 * specify --force or --picky. */
	if (cmdline_option_fix && !is_current_version(version)) {
		if (cmdline_option_picky || force) {
			fprintf(stderr, "WARNING: the force or picky option is specified; "
					"ignoring version mismatch for verify --fix\n");
		} else {
			fprintf(stderr, "ERROR: Fixing to a different version requires "
					"--force or --picky\n");
			ret = SWUPD_INVALID_OPTION;
			goto clean_and_exit;
		}
	}

	ret = add_included_manifests(official_manifest, &subs);
	/* if one or more of the installed bundles were not found in the manifest,
	 * continue only if --force was used since the bundles could be removed */
	if (ret) {
		if (ret == -add_sub_BADNAME) {
			if (force) {
				if (cmdline_option_picky && cmdline_option_fix) {
					fprintf(stderr, "WARNING: One or more installed bundles that are not "
							"available at version %d will be removed.\n",
						version);
				} else if (cmdline_option_picky && !cmdline_option_fix) {
					fprintf(stderr, "WARNING: One or more installed bundles are not "
							"available at version %d.\n",
						version);
				}
				ret = SWUPD_OK;
			} else {
				fprintf(stderr, "Unable to verify, one or more currently installed bundles "
						"are not available at version %d. Use --force to override.\n",
					version);
				ret = SWUPD_INVALID_BUNDLE;
				goto clean_and_exit;
			}
		} else {
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto clean_and_exit;
		}
	}

	set_subscription_versions(official_manifest, NULL, &subs);
load_submanifests:
	official_manifest->submanifests = recurse_manifest(official_manifest, subs, NULL, false, NULL);
	if (!official_manifest->submanifests) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading MoM sub-manifests\n", retries);
			goto load_submanifests;
		}
		fprintf(stderr, "Error: Cannot load MoM sub-manifests\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto clean_and_exit;
	}
	timelist_timer_stop(global_times);
	timelist_timer_start(global_times, "Consolidate files from bundles");
	official_manifest->files = files_from_bundles(official_manifest->submanifests);
	official_manifest->files = consolidate_files(official_manifest->files);
	timelist_timer_stop(global_times);
	timelist_timer_start(global_times, "Get required files");

	/* get the initial number of files to be inspected */
	counts.checked = list_len(official_manifest->files);

	/* when fixing or installing we need input files. */
	if (cmdline_option_fix || cmdline_option_install) {
		ret = get_required_files(official_manifest, subs);
		if (ret != 0) {
			ret = SWUPD_COULDNT_DOWNLOAD_FILE;
			goto clean_and_exit;
		}
	}
	timelist_timer_stop(global_times);

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

	if (cmdline_option_fix || cmdline_option_install) {
		/*
		 * Next put the files in place that are missing completely.
		 * This is to avoid updating a symlink to a library before the new full file
		 * is already there. It's also the most safe operation, adding files rarely
		 * has unintended side effect. So lets do the safest thing first.
		 */
		bool repair = true;

		timelist_timer_start(global_times, "Add missing files");
		fprintf(stderr, "Adding any missing files\n");
		add_missing_files(official_manifest, repair);
		timelist_timer_stop(global_times);
	}

	if (cmdline_option_quick) {
		/* quick only replaces missing files, so it is done here */
		goto brick_the_system_and_clean_curl;
	}

	if (cmdline_option_fix) {
		bool repair = true;

		timelist_timer_start(global_times, "Fixing modified files");
		fprintf(stderr, "Fixing modified files\n");
		deal_with_hash_mismatches(official_manifest, repair);

		/* removing files could be risky, so only do it if the
		 * prior phases had no problems */
		if ((counts.not_fixed == 0) && (counts.not_replaced == 0)) {
			remove_orphaned_files(official_manifest, repair);
		}
		if (cmdline_option_picky) {
			char *start = mk_full_filename(path_prefix, cmdline_option_picky_tree);
			fprintf(stderr, "--picky removing extra files under %s\n", start);
			ret = walk_tree(official_manifest, start, true, picky_whitelist, &counts);
			free_string(&start);
		}
		timelist_timer_stop(global_times);
	} else if (cmdline_option_picky) {
		char *start = mk_full_filename(path_prefix, cmdline_option_picky_tree);
		fprintf(stderr, "Generating list of extra files under %s\n", start);
		ret = walk_tree(official_manifest, start, false, picky_whitelist, &counts);
		free_string(&start);
	} else {
		bool repair = false;

		fprintf(stderr, "Verifying files\n");
		add_missing_files(official_manifest, repair);
		/* quick only checks for missing files, so it is done here */
		if (!cmdline_option_quick) {
			deal_with_hash_mismatches(official_manifest, repair);
			remove_orphaned_files(official_manifest, repair);
		}
	}

brick_the_system_and_clean_curl:
	/* clean up */

	/*
	 * naming convention: All exit goto labels must follow the "brick_the_system_and_FOO:" pattern
	 */

	/* report a summary of what we managed to do and not do */
	fprintf(stderr, "Inspected %i file%s\n", counts.checked, (counts.checked == 1 ? "" : "s"));

	if (counts.missing) {
		fprintf(stderr, "  %i file%s %s missing\n", counts.missing, (counts.missing > 1 ? "s" : ""), (counts.missing > 1 ? "were" : "was"));
		if (cmdline_option_fix || cmdline_option_install) {
			fprintf(stderr, "    %i of %i missing files were replaced\n", counts.replaced, counts.missing);
			fprintf(stderr, "    %i of %i missing files were not replaced\n", counts.not_replaced, counts.missing);
		}
	}

	if (counts.mismatch) {
		fprintf(stderr, "  %i file%s did not match\n", counts.mismatch, (counts.mismatch > 1 ? "s" : ""));
		if (cmdline_option_fix) {
			fprintf(stderr, "    %i of %i files were fixed\n", counts.fixed, counts.mismatch);
			fprintf(stderr, "    %i of %i files were not fixed\n", counts.not_fixed, counts.mismatch);
		}
	}

	if (counts.extraneous) {
		fprintf(stderr, "  %i file%s found which should be deleted\n", counts.extraneous, (counts.extraneous > 1 ? "s" : ""));
		if (cmdline_option_fix) {
			fprintf(stderr, "    %i of %i files were deleted\n", counts.deleted, counts.extraneous);
			fprintf(stderr, "    %i of %i files were not deleted\n", counts.not_deleted, counts.extraneous);
		}
	}

	if (cmdline_option_fix || cmdline_option_install) {
		// always run in a fix or install case
		need_update_boot = true;
		need_update_bootloader = true;
		timelist_timer_start(global_times, "Run Scripts");
		run_scripts(false);
		timelist_timer_stop(global_times);
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

clean_and_exit:
	free_manifest(official_manifest);
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
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
		  "file_extraneous_count=%d\n",
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
		  counts.extraneous);
	if (ret == SWUPD_OK) {
		if (cmdline_option_fix || cmdline_option_install) {
			fprintf(stderr, "Fix successful\n");
		} else {
			/* This is just a verification */
			fprintf(stderr, "Verify successful\n");
		}
	} else {
		if (cmdline_option_fix || cmdline_option_install) {
			fprintf(stderr, "Error: Fix did not fully succeed\n");
		} else {
			/* This is just a verification */
			fprintf(stderr, "Error: Verify did not fully succeed\n");
		}
	}

	timelist_print_stats(global_times);
	free_subscriptions(&subs);
	swupd_deinit();

clean_args_and_exit:
	if (picky_whitelist) {
		regfree(picky_whitelist);
		picky_whitelist = NULL;
	}

	return ret;
}
