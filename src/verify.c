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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>

#include "config.h"
#include "swupd.h"
#include "signature.h"

static bool cmdline_option_fix = false;
static bool cmdline_option_install = false;
static bool cmdline_option_quick = false;

static int version;

/* Count of how many files we managed to not fix */
static int file_checked_count;
static int file_missing_count;
static int file_replaced_count;
static int file_not_replaced_count;
static int file_mismatch_count;
static int file_fixed_count;
static int file_not_fixed_count;
static int file_extraneous_count;
static int file_deleted_count;
static int file_not_deleted_count;

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "manifest", required_argument, 0, 'm' },
	{ "path", required_argument, 0, 'p' },
	{ "url", required_argument, 0, 'u' },
	{ "port", required_argument, 0, 'P' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "fix", no_argument, 0, 'f' },
	{ "install", no_argument, 0, 'i' },
	{ "format", required_argument, 0, 'F' },
	{ "quick", no_argument, 0, 'q' },
	{ "force", no_argument, 0, 'x' },
	{ 0, 0, 0, 0 }
};

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [OPTION...]\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n\n");
	printf("Application Options:\n");
	printf("   -m, --manifest=M        Verify against manifest version M\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	printf("   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	printf("   -v, --versionurl=[URL]  RFC-3986 encoded url for version file downloads\n");
	printf("   -f, --fix               Fix local issues relative to server manifest (will not modify ignored files)\n");
	printf("   -i, --install           Similar to \"--fix\" but optimized for install all files to empty directory\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -q, --quick             Don't compare hashes, only fix missing files\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("\n");
}

static bool parse_options(int argc, char **argv)
{
	int opt;

	//set default initial values
	set_format_string(NULL);

	while ((opt = getopt_long(argc, argv, "hxm:p:u:P:c:v:fiF:q", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'm':
			if (sscanf(optarg, "%i", &version) != 1) {
				printf("Invalid --manifest argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			if (path_prefix) { /* multiple -p options */
				free(path_prefix);
			}
			string_or_die(&path_prefix, "%s", optarg);
			break;
		case 'u':
			if (!optarg) {
				printf("Invalid --url argument\n\n");
				goto err;
			}
			if (version_server_urls[0]) {
				free(version_server_urls[0]);
			}
			if (content_server_urls[0]) {
				free(content_server_urls[0]);
			}
			string_or_die(&version_server_urls[0], "%s", optarg);
			string_or_die(&content_server_urls[0], "%s", optarg);
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				printf("Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'c':
			if (!optarg) {
				printf("Invalid --contenturl argument\n\n");
				goto err;
			}
			if (content_server_urls[0]) {
				free(content_server_urls[0]);
			}
			string_or_die(&content_server_urls[0], "%s", optarg);
			break;
		case 'v':
			if (!optarg) {
				printf("Invalid --versionurl argument\n\n");
				goto err;
			}
			if (version_server_urls[0]) {
				free(version_server_urls[0]);
			}
			string_or_die(&version_server_urls[0], "%s", optarg);
			break;
		case 'f':
			cmdline_option_fix = true;
			break;
		case 'i':
			cmdline_option_install = true;
			cmdline_option_quick = true;
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				printf("Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'q':
			cmdline_option_quick = true;
			break;
		case 'x':
			force = true;
			break;
		default:
			printf("Unrecognized option\n\n");
			goto err;
		}
	}

	if (cmdline_option_install) {
		if (version == 0) {
			printf("--install option requires -m version option\n");
			return false;
		}
		if (path_prefix == NULL) {
			printf("--install option requires --path option\n");
			return false;
		}
		if (cmdline_option_fix) {
			printf("--install and --fix options are mutually exclusive\n");
			return false;
		}
	}

	if (!init_globals()) {
		return false;
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

static bool hash_needs_work(struct file *file, char *hash)
{
	if (cmdline_option_quick) {
		if (hash_is_zeros(hash)) {
			return true;
		} else {
			return false;
		}
	} else {
		if (hash_compare(file->hash, hash)) {
			return false;
		} else {
			return true;
		}
	}
}

static int get_all_files(int version, struct manifest *official_manifest)
{
	int ret;
	struct list *iter;

	/* for install we need everything so synchronously download zero packs */
	ret = download_subscribed_packs(0, version, true);
	if (ret < 0) { // require zero pack
		/* If we hit this point, we know we have a network connection, therefore
		 * 	the error is server-side. This is also a critical error, so detailed
		 * 	logging needed */
		printf("zero pack downloads failed. \n");
		printf("Failed - Server-side error, cannot download necessary files\n");
		return ret;
	}
	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;

		file = iter->data;
		iter = iter->next;

		if (file->is_deleted) {
			continue;
		}

		file_checked_count++;
	}
	return 0;
}

static struct list *download_loop(struct list *files, int isfailed)
{
	int ret;
	struct file local;
	struct list *iter;

	iter = list_head(files);
	while (iter) {
		struct file *file;
		char *fullname;

		file = iter->data;
		iter = iter->next;

		if (file->is_deleted) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, file->filename);
		if (fullname == NULL) {
			abort();
		}

		memset(&local, 0, sizeof(struct file));
		local.filename = file->filename;
		populate_file_struct(&local, fullname);

		if (cmdline_option_quick) {
			ret = compute_hash_lazy(&local, fullname);
		} else {
			ret = compute_hash(&local, fullname);
		}
		if (ret != 0) {
			free(fullname);
			continue;
		}

		if (hash_needs_work(file, local.hash)) {
			full_download(file);
		} else {
			/* mark the file as good to save time later */
			file->do_not_update = 1;
		}
		free(fullname);
	}

	if (isfailed) {
		list_free_list(files);
	}

	return end_full_download();
}

static int get_missing_files(struct manifest *official_manifest)
{
	int ret;
	struct list *failed = NULL;
	int retries = 0;  /* We only want to go through the download loop once */
	int timeout = 10; /* Amount of seconds for first download retry */

RETRY_DOWNLOADS:
	/* when fixing (not installing): queue download and mark any files
	 * which are already verified OK */
	ret = start_full_download(true);
	if (ret != 0) {
		/* If we hit this point, the network is accessible but we were
		 * 	unable to download the needed files. This is a terminal error
		 * 	and we need good logging */
		printf("Error: Unable to download neccessary files for this OS release\n");
		return ret;
	}

	if (failed != NULL) {
		failed = download_loop(failed, 1);
	} else {
		failed = download_loop(official_manifest->files, 0);
	}

	/* Set retries only if failed downloads exist, and only retry a fixed
	   amount of times */
	if (list_head(failed) != NULL && retries < MAX_TRIES) {
		increment_retries(&retries, &timeout);
		printf("Starting download retry #%d\n", retries);
		clean_curl_multi_queue();
		goto RETRY_DOWNLOADS;
	}

	if (retries >= MAX_TRIES) {
		printf("ERROR: Could not download all files, aborting update\n");
		list_free_list(failed);
		return -1;
	}

	return 0;
}

/* allow optimization of install case */
static int get_required_files(int version, struct manifest *official_manifest)
{
	if (cmdline_option_install) {
		return get_all_files(version, official_manifest);
	}

	if (cmdline_option_fix) {
		return get_missing_files(official_manifest);
	}

	return 0;
}

/* for each missing but expected file, (re)add the file */
static void add_missing_files(struct manifest *official_manifest)
{
	int ret;
	struct file local;
	struct list *iter;

	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;
		char *fullname;

		file = iter->data;
		iter = iter->next;

		if ((file->is_deleted) ||
		    (file->do_not_update)) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, file->filename);
		if (fullname == NULL) {
			abort();
		}
		memset(&local, 0, sizeof(struct file));
		local.filename = file->filename;
		populate_file_struct(&local, fullname);
		ret = compute_hash_lazy(&local, fullname);
		if (ret != 0) {
			file_not_replaced_count++;
			free(fullname);
			continue;
		}

		/* compare the hash and report mismatch */
		if (hash_is_zeros(local.hash)) {
			file_missing_count++;
			if (cmdline_option_install == false) {
				printf("Missing file: %s\n", fullname);
			}
		} else {
			free(fullname);
			continue;
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
			file_not_replaced_count++;
			printf("\tnot fixed\n");
		} else {
			file_replaced_count++;
			file->do_not_update = 1;
			if (cmdline_option_install == false) {
				printf("\tfixed\n");
			}
		}
		free(fullname);
	}
}

static void deal_with_hash_mismatches(struct manifest *official_manifest, bool repair)
{
	int ret;
	struct list *iter;

	/* for each expected and present file which hash-mismatches vs the manifest, replace the file */
	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;
		char *fullname;

		file = iter->data;
		iter = iter->next;

		if (file->is_deleted ||
		    ignore(file, false)) {
			continue;
		}

		file_checked_count++;

		// do_not_update set by earlier check, so account as checked
		if (file->do_not_update) {
			continue;
		}

		/* compare the hash and report mismatch */
		fullname = mk_full_filename(path_prefix, file->filename);
		if (fullname == NULL) {
			abort();
		}
		if (verify_file(file, fullname)) {
			free(fullname);
			continue;
		} else {
			file_mismatch_count++;
			printf("Hash mismatch for file: %s\n", fullname);
		}

		/* if not repairing, we're done */
		if (!repair) {
			free(fullname);
			continue;
		}

		/* install the new file (on miscompare + fix) */
		ret = do_staging(file, official_manifest);
		if (ret == 0) {
			rename_staged_file_to_final(file);
		}

		/* at the end of all this, verify the hash again to judge success */
		if (verify_file(file, fullname)) {
			file_fixed_count++;
			printf("\tfixed\n");
		} else {
			file_not_fixed_count++;
			printf("\tnot fixed\n");
		}
		free(fullname);
	}
}

static void remove_orphaned_files(struct manifest *official_manifest)
{
	int ret;
	struct list *iter;

	iter = list_head(official_manifest->files);
	while (iter) {
		struct file *file;
		char *fullname;
		struct stat sb;

		file = iter->data;
		iter = iter->next;

		if ((!file->is_deleted) ||
		    (file->is_config)) {
			continue;
		}

		fullname = mk_full_filename(path_prefix, file->filename);
		if (fullname == NULL) {
			abort();
		}

		if (lstat(fullname, &sb) != 0) {
			/* correctly, the file is not present */
			free(fullname);
			continue;
		}

		file_extraneous_count++;
		if (is_dirname_link(fullname)) {
			free(fullname);
			continue;
		}

		if (!file->is_dir) {
			ret = unlink(fullname);
			if (!ret && errno != ENOENT && errno != EISDIR) {
				printf("Failed to remove %s (%i: %s)\n", fullname, errno, strerror(errno));
				file_not_deleted_count++;
			} else {
				printf("Deleted %s \n", fullname);
				file_deleted_count++;
			}
		} else {
			ret = rmdir(fullname);
			if (ret) {
				file_not_deleted_count++;
				if (errno != ENOTEMPTY) {
					printf("Failed to remove empty folder %s (%i: %s)\n",
					       fullname, errno, strerror(errno));
				} else {
					//FIXME: Add force removal option?
					printf("Couldn't remove directory containing untracked files: %s\n", fullname);
				}
			}
		}
		free(fullname);
	}
}

/* This function does a simple verification of files listed in the
 * subscribed bundle manifests.  If the optional "fix" or "install" parameter
 * is specified, the disk will be modified at each point during the
 * sequential comparison of manifest files to disk files, where the disk is
 * found to not match the manifest.  This is notably different from update,
 * which attempts to atomically (or nearly atomically) activate a set of
 * pre-computed and validated staged changes as a group. */
int verify_main(int argc, char **argv)
{
	struct manifest *official_manifest = NULL;
	int ret;
	int lock_fd;

	copyright_header("software verify");

	if (!parse_options(argc, argv) ||
	    create_required_dirs()) {
		free_globals();
		return EXIT_FAILURE;
	}

	/* parse command line options */
	assert(argc >= 0);
	assert(argv != NULL);

	/* Gather current manifests */
	if (!version) {
		version = read_version_from_subvol_file(path_prefix);
		if (!version) {
			printf("Cannot determine current version\n");
			free_globals();
			return EXIT_FAILURE;
		}
	}

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		printf("Failed verify initialization, exiting now.\n");
		return ret;
	}

	printf("Verifying version %i\n", version);

	if (!check_network()) {
		printf("Error: Network issue, unable to download manifest\n");
		v_lockfile(lock_fd);
		return EXIT_FAILURE;
	}

	read_subscriptions_alt();
	get_mounted_directories();

	/*
	 * FIXME: We need a command line option to override this in case the
	 * certificate is hosed and the admin knows it and wants to recover.
	 */
	if (!signature_initialize(UPDATE_CA_CERTS_PATH "/" SIGNATURE_CA_CERT)) {
		printf("Can't initialize the SSL certificates\n");
		goto brick_the_system_and_clean_curl;
	}

	ret = rm_staging_dir_contents("download");
	if (ret != 0) {
		printf("Failed to remove prior downloads, carrying on anyway\n");
	}

	ret = load_manifests(version, version, "MoM", NULL, &official_manifest);

	if (ret != 0) {
		/* This should never get hit, since network issues are identified by the
		 * 	check_network() call earlier */
		printf("Unable to download %d Manifest.MoM\n", version);
		goto brick_the_system_and_clean_curl;
	}

	subscription_versions_from_MoM(official_manifest, 0);
	recurse_manifest(official_manifest, NULL);
	consolidate_submanifests(official_manifest);

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
		/* when fixing or installing we need input files. */
		ret = get_required_files(version, official_manifest);
		if (ret != 0) {
			goto brick_the_system_and_clean_curl;
		}

		/*
		 * Next put the files in place that are missing completely.
		 * This is to avoid updating a symlink to a library before the new full file
		 * is already there. It's also the most safe operation, adding files rarely
		 * has unintended side effect. So lets do the safest thing first.
		 */
		printf("Adding any missing files\n");
		add_missing_files(official_manifest);
	}

	if (cmdline_option_quick) {
		/* quick only replaces missing files, so it is done here */
		goto brick_the_system_and_clean_curl;
	}

	if (cmdline_option_fix) {
		bool repair = true;

		printf("Fixing modified files\n");
		deal_with_hash_mismatches(official_manifest, repair);

		/* removing files could be risky, so only do it if the
		 * prior phases had no problems */
		if ((file_not_fixed_count == 0) && (file_not_replaced_count == 0)) {
			remove_orphaned_files(official_manifest);
		}
	} else {
		bool repair = false;

		printf("Verifying files\n");
		deal_with_hash_mismatches(official_manifest, repair);
	}

brick_the_system_and_clean_curl:
	/* clean up */

	/*
	 * naming convention: All exit goto labels must follow the "brick_the_system_and_FOO:" pattern
	 */

	/* report a summary of what we managed to do and not do */
	printf("Inspected %i files\n", file_checked_count);

	if (cmdline_option_fix || cmdline_option_install) {
		printf("  %i files were missing\n", file_missing_count);
		if (file_missing_count) {
			printf("    %i of %i missing files were replaced\n", file_replaced_count, file_missing_count);
			printf("    %i of %i missing files were not replaced\n", file_not_replaced_count, file_missing_count);
		}
	}

	if (!cmdline_option_quick && file_mismatch_count > 0) {
		printf("  %i files did not match\n", file_mismatch_count);
		if (cmdline_option_fix) {
			printf("    %i of %i files were fixed\n", file_fixed_count, file_mismatch_count);
			printf("    %i of %i files were not fixed\n", file_not_fixed_count, file_mismatch_count);
		}
	}

	if ((file_not_fixed_count == 0) && (file_not_replaced_count == 0) &&
	    cmdline_option_fix && !cmdline_option_quick) {
		printf("  %i files found which should be deleted\n", file_extraneous_count);
		if (file_extraneous_count) {
			printf("    %i of %i files were deleted\n", file_deleted_count, file_extraneous_count);
			printf("    %i of %i files were not deleted\n", file_not_deleted_count, file_extraneous_count);
		}
	}

	if (cmdline_option_fix || cmdline_option_install) {
		// always run in a fix or install case
		need_update_boot = true;
		need_update_bootloader = true;
		run_scripts();
	}

	sync();

	if ((file_not_fixed_count == 0) &&
	    (file_not_replaced_count == 0) &&
	    (file_not_deleted_count == 0)) {
		ret = EXIT_SUCCESS;
	} else {
		ret = EXIT_FAILURE;
	}

	/* this concludes the critical section, after this point it's clean up time, the disk content is finished and final */

	if (ret == EXIT_SUCCESS) {
		if (cmdline_option_fix || cmdline_option_install) {
			printf("Fix successful\n");
		} else {
			/* This is just a verification */
			printf("Verify successful\n");
		}
	} else {
		if (cmdline_option_fix || cmdline_option_install) {
			printf("Error: Fix did not fully succeed\n");
		} else {
			/* This is just a verification */
			printf("Error: Verify did not fully succeed\n");
		}
	}
	swupd_curl_cleanup();
	free_subscriptions();
	free_manifest(official_manifest);

	v_lockfile(lock_fd);
	dump_file_descriptor_leaks();
	free_globals();
	return ret;
}
