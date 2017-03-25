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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

static bool cmdline_option_fix = false;
static bool cmdline_option_install = false;
static bool cmdline_option_quick = false;

static int version = 0;

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
	{ "ignore", no_argument, 0, 'I' },
	{ "format", required_argument, 0, 'F' },
	{ "quick", no_argument, 0, 'q' },
	{ "force", no_argument, 0, 'x' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "statedir", required_argument, 0, 'S' },
	{ "certpath", required_argument, 0, 'C' },
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
	printf("   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	printf("   -I, --ignore-certtime   Ignore system/certificate time when validating signature\n");
	printf("   -S, --statedir          Specify alternate swupd state directory\n");
	printf("   -C, --certpath          Specify alternate path to swupd certificates\n");
	printf("\n");
}

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxnIm:p:u:P:c:v:fiF:qS:C:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'm':
			if (strcmp("latest", optarg) == 0) {
				version = -1;
			} else if (sscanf(optarg, "%i", &version) != 1) {
				printf("Invalid --manifest argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 'u':
			if (!optarg) {
				printf("Invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
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
			set_content_url(optarg);
			break;
		case 'v':
			if (!optarg) {
				printf("Invalid --versionurl argument\n\n");
				goto err;
			}
			set_version_url(optarg);
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
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				printf("Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'q':
			cmdline_option_quick = true;
			break;
		case 'x':
			force = true;
			break;
		case 'n':
			sigcheck = false;
			break;
		case 'I':
			timecheck = false;
			break;
		case 'C':
			if (!optarg) {
				printf("Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
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
	} else if (version == -1) {
		printf("-m latest only supported with --install\n");
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
		return (hash_is_zeros(hash));
	} else {
		return (!hash_equal(file->hash, hash));
	}
}

static int get_all_files(struct manifest *official_manifest, struct list *subs)
{
	int ret;
	struct list *iter;

	/* for install we need everything so synchronously download zero packs */
	ret = download_subscribed_packs(subs, true);
	if (ret < 0) { // require zero pack
		/* If we hit this point, we know we have a network connection, therefore
		 * 	the error is server-side. This is also a critical error, so detailed
		 * 	logging needed */
		printf("zero pack downloads failed. \n");
		printf("Failed - Server-side error, cannot download necessary files\n");
		return -ENOSWUPDSERVER;
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
		return -EFULLDOWNLOAD;
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
		return -EFULLDOWNLOAD;
	}

	return 0;
}

/* allow optimization of install case */
static int get_required_files(struct manifest *official_manifest, struct list *subs)
{
	if (cmdline_option_install) {
		return get_all_files(official_manifest, subs);
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

		// Note: boot files not marked as deleted are candidates for verify/fix
		if (file->is_deleted ||
		    ignore(file)) {
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

		if ((!file->is_deleted) ||
		    (file->is_config)) {
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
		if (fullname == NULL) {
			abort();
		}

		if (lstat(fullname, &sb) != 0) {
			/* correctly, the file is not present */
			free(fullname);
			continue;
		}

		file_extraneous_count++;

		fd = get_dirfd_path(fullname);
		if (fd < 0) {
			printf("Not safe to delete: %s\n", fullname);
			free(fullname);
			file_not_deleted_count++;
			continue;
		}

		base = basename(fullname);

		if (!S_ISDIR(sb.st_mode)) {
			ret = unlinkat(fd, base, 0);
			if (ret && errno != ENOENT) {
				printf("Failed to remove %s (%i: %s)\n", fullname, errno, strerror(errno));
				file_not_deleted_count++;
			} else {
				printf("Deleted %s\n", fullname);
				file_deleted_count++;
			}
		} else {
			ret = unlinkat(fd, base, AT_REMOVEDIR);
			if (ret) {
				file_not_deleted_count++;
				if (errno != ENOTEMPTY) {
					printf("Failed to remove empty folder %s (%i: %s)\n",
					       fullname, errno, strerror(errno));
				} else {
					//FIXME: Add force removal option?
					printf("Couldn't remove directory containing untracked files: %s\n", fullname);
				}
			} else {
				printf("Deleted %s\n", fullname);
				file_deleted_count++;
			}
		}
		free(fullname);
		close(fd);
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
	struct list *subs = NULL;

	copyright_header("software verify");

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	/* parse command line options */
	assert(argc >= 0);
	assert(argv != NULL);

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		printf("Failed verify initialization, exiting now.\n");
		return ret;
	}

	/* Gather current manifests */
	if (!version) {
		version = get_current_version(path_prefix);
		if (version < 0) {
			printf("Error: Unable to determine current OS version\n");
			ret = ECURRENT_VERSION;
			goto clean_and_exit;
		}
	}

	if (version == -1) {
		version = get_latest_version();
		if (version < 0) {
			printf("Unable to get latest version for install\n");
			ret = EXIT_FAILURE;
			goto clean_and_exit;
		}
	}

	printf("Verifying version %i\n", version);

	if (!check_network()) {
		printf("Error: Network issue, unable to download manifest\n");
		ret = ENOSWUPDSERVER;
		goto clean_and_exit;
	}

	read_subscriptions_alt(&subs);

	/*
	 * FIXME: We need a command line option to override this in case the
	 * certificate is hosed and the admin knows it and wants to recover.
	 */

	ret = rm_staging_dir_contents("download");
	if (ret != 0) {
		printf("Failed to remove prior downloads, carrying on anyway\n");
	}

	official_manifest = load_mom(version);

	if (!official_manifest) {
		/* This is hit when or if an OS version is specified for --fix which
		 * is not available, or if there is a server error and a manifest is
		 * not provided.
		 */
		printf("Unable to download/verify %d Manifest.MoM\n", version);
		ret = EMOM_NOTFOUND;

		/* No repair is possible without a manifest, nor is accurate reporting
		 * of the state of the system. Therefore cleanup, report failure and exit
		 */
		goto clean_and_exit;
	}

	ret = add_included_manifests(official_manifest, &subs);
	if (ret) {
		ret = EMANIFEST_LOAD;
		goto clean_and_exit;
	}

	set_subscription_versions(official_manifest, NULL, &subs);

	official_manifest->submanifests = recurse_manifest(official_manifest, subs, NULL);
	if (!official_manifest->submanifests) {
		printf("Error: Cannot load MoM sub-manifests\n");
		ret = ERECURSE_MANIFEST;
		goto clean_and_exit;
	}
	official_manifest->files = files_from_bundles(official_manifest->submanifests);

	official_manifest->files = consolidate_files(official_manifest->files);

	/* when fixing or installing we need input files. */
	if (cmdline_option_fix || cmdline_option_install) {
		ret = get_required_files(official_manifest, subs);
		if (ret != 0) {
			ret = -ret;
			goto clean_and_exit;
		}
	}

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
	free_manifest(official_manifest);

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

clean_and_exit:
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
		  file_replaced_count,
		  file_not_replaced_count,
		  file_missing_count,
		  file_fixed_count,
		  file_not_fixed_count,
		  file_deleted_count,
		  file_not_deleted_count,
		  file_mismatch_count,
		  file_extraneous_count);
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

	swupd_deinit(lock_fd, &subs);

	return ret;
}
