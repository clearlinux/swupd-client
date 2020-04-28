/*
 *   Software Updater - client side
 *
 *      Copyright © 2018 Intel Corporation.
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

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "swupd.h"

#define FLAG_ALL 2000
#define FLAG_DRY_RUN 2001

static void print_help(void)
{
	print("Remove cached content used for updates from state directory\n\n");
	print("Usage:\n");
	print("   swupd clean [OPTION...]\n\n");

	global_print_help();

	print(
	    "Options:\n"
	    "   --all                   Remove all the content including recent metadata\n"
	    "   --dry-run               Just print files that would be removed\n"
	    "\n");
}

static struct {
	int all;
	int dry_run;
} options;

static struct {
	int files_removed;
	long bytes_removed;
} stats;

int clean_get_stats(void)
{
	return stats.files_removed;
}

static struct timespec now;

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "all", no_argument, 0, FLAG_ALL },
	{ "dry-run", no_argument, 0, FLAG_DRY_RUN },
};

static bool parse_opt(int opt, char *optarg UNUSED_PARAM)
{
	switch (opt) {
	case FLAG_ALL:
		options.all = true;
		return true;
	case FLAG_DRY_RUN:
		options.dry_run = true;
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

	if (argc > optind) {
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

typedef bool(remove_predicate_func)(const char *dir, const struct dirent *entry);

/* Remove files from path for which pred returns true.
 * Currently it doesn't recursively remove directories.
 */
static enum swupd_code remove_if(const char *path, bool dry_run, remove_predicate_func pred)
{
	int ret = SWUPD_OK;
	DIR *dir;
	long size, hardlink_count;

	dir = opendir(path);
	if (!dir) {
		return SWUPD_COULDNT_LIST_DIR;
	}

	char *file = NULL;

	while (true) {
		FREE(file);
		ret = SWUPD_OK;

		/* Reset errno to distinguish between a previous
		 * failure and the end of stream. */
		errno = 0;
		struct dirent *entry;
		entry = readdir(dir);
		if (!entry) {
			if (errno) {
				ret = SWUPD_COULDNT_LIST_DIR;
			}
			break;
		}

		if (!str_cmp(entry->d_name, ".") || !str_cmp(entry->d_name, "..")) {
			continue;
		}

		file = sys_path_join("%s/%s", path, entry->d_name);

		if (!pred(path, entry)) {
			continue;
		}

		hardlink_count = sys_file_hardlink_count(file);
		if (hardlink_count == 1) {
			/* a file being removed (unlinked) may have many hardlinks which may
			 * stay in the system, so the only files being trully removed are
			 * those which only have one inode left in the system.
			 * This also means that when doing dry-run we can only guess how
			 * much space we would free since we cannot know what inodes will
			 * have all their hardlinks removed */
			size = sys_get_file_size(file);
		} else {
			size = 0;
			if (hardlink_count < 0) {
				warn("Couldn't get file size: %s\n", file);
			}
		}

		if (dry_run) {
			print("%s\n", file);
		} else {
			ret = sys_rm(file);
			if (ret != 0) {
				warn("couldn't remove file %s: %s\n", file, strerror(errno));
			}
		}
		if (ret == 0) {
			stats.files_removed++;
			stats.bytes_removed += size;
		}
	}

	closedir(dir);
	return ret;
}

static bool is_fullfile(const char UNUSED_PARAM *dir, const struct dirent *entry)
{
	return str_len(entry->d_name) == (SWUPD_HASH_LEN - 1);
}

static bool is_pack_indicator(const char UNUSED_PARAM *dir, const struct dirent *entry)
{
	static const char prefix[] = "pack-";
	static const char suffix[] = ".tar";
	static const size_t prefix_len = sizeof(prefix) - 1;
	static const size_t suffix_len = sizeof(suffix) - 1;

	const char *name = entry->d_name;
	size_t len = str_len(name);
	if (len < (prefix_len + suffix_len)) {
		return false;
	}
	return !memcmp(name, prefix, prefix_len) && !memcmp(name + len - suffix_len, suffix, suffix_len);
}

static bool is_all_digits(const char *s)
{
	for (; *s; s++) {
		if (!isdigit(*s)) {
			return false;
		}
	}
	return true;
}

static bool is_all_xdigits(const char *s)
{
	for (; *s; s++) {
		if (!isxdigit(*s)) {
			return false;
		}
	}
	return true;
}

static bool is_manifest(const char UNUSED_PARAM *dir, const struct dirent *entry)
{
	return !str_starts_with(entry->d_name, "Manifest.");
}

static bool is_hashed_manifest(const char UNUSED_PARAM *dir, const struct dirent *entry)
{
	/* check that this has the manifest prefix */
	if (!is_manifest(dir, entry)) {
		return false;
	}

	int counter = 0;
	int start = sizeof("Manifest.");
	const char *ename = entry->d_name + start;

	/* check for the correct number of '.' characters (i.e.
	 * Manifest.bundlename.hashvalue). */
	for (ename = entry->d_name + start; *ename; ename++) {
		if (*ename == '.') {
			counter++;
		}
	}
	if (counter != 1) {
		return false;
	}

	/* grab hash suffix by splitting on last '.' then iterating past it (+ 1) */
	return is_all_xdigits(strrchr(entry->d_name + start, '.') + 1);
}

static bool is_manifest_delta(const char UNUSED_PARAM *dir, const struct dirent *entry)
{
	return !str_starts_with(entry->d_name, "Manifest-");
}

static char *read_mom_contents(int version)
{
	char *mom_path = NULL;
	string_or_die(&mom_path, "%s/%d/Manifest.MoM", globals.state_dir, version);
	FILE *f = fopen(mom_path, "r");
	FREE(mom_path);
	if (!f) {
		/* This is a best effort. */
		return NULL;
	}

	char *contents = NULL;
	int fd = fileno(f);
	if (fd == -1) {
		goto end;
	}

	int ret;
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret != 0) {
		goto end;
	}

	contents = malloc(stat.st_size + 1);
	ON_NULL_ABORT(contents);

	ret = fread(contents, stat.st_size, 1, f);
	if (ret != 1) {
		FREE(contents);
		contents = NULL;
	} else {
		contents[stat.st_size] = 0;
	}

end:
	fclose(f);
	return contents;
}

static enum swupd_code clean_staged_manifests(const char *path, bool dry_run, bool all)
{
	DIR *dir;
	long size;

	dir = opendir(path);
	if (!dir) {
		return SWUPD_COULDNT_LIST_DIR;
	}

	/* NOTE: Currently Manifest files have their timestamp from generation
	 * preserved. */

	/* When --all is not used, keep the Manifests used by the current OS version. This
	 * ensures that a regular 'clean' won't make 'search' redownload files. */
	char *mom_contents = NULL;
	if (!all) {
		int current_version = get_current_version(globals.path_prefix);
		if (current_version < 0) {
			warn("Unable to determine current OS version\n");
		} else {
			mom_contents = read_mom_contents(current_version);
		}
	}

	int ret = SWUPD_OK;
	while (true) {
		/* Reset errno to properly identify the end of stream. */
		errno = 0;
		struct dirent *entry;
		entry = readdir(dir);
		if (!entry) {
			if (errno) {
				ret = SWUPD_COULDNT_LIST_DIR;
			}
			break;
		}

		const char *name = entry->d_name;
		if (!str_cmp(name, ".") || !str_cmp(name, "..")) {
			continue;
		}
		if (!is_all_digits(name)) {
			continue;
		}

		char *version_dir = sys_path_join("%s/%s", globals.state_dir, name);

		/* This is not precise: it may keep Manifest files that we don't use, and
		 * also will keep the previous version. If that extra precision is
		 * required we should parse the manifest. */
		if (mom_contents && strstr(mom_contents, name)) {
			/* Remove all hash-hint manifest files. */
			ret = remove_if(version_dir, dry_run, is_hashed_manifest);
		} else {
			/* Remove all manifest files, including hash-hints */
			ret = remove_if(version_dir, dry_run, is_manifest);
		}

		/* Remove empty dirs if possible. */
		size = sys_get_file_size(version_dir);
		if (size < 0) {
			size = 0;
		}
		if (!rmdir(version_dir) || (dry_run && all)) {
			stats.bytes_removed += size;
		}

		FREE(version_dir);
		if (ret != 0) {
			break;
		}
	}

	FREE(mom_contents);
	closedir(dir);
	return ret;
}

enum swupd_code clean_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_clean = 0;
	char *bytes_removed_pretty = NULL;

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

	progress_init_steps("clean", steps_in_clean);

	/* NOTE: Delete specific file patterns to avoid disasters in case some paths are
	 * set incorrectly. */

	/* Staged files. */

	/* TODO: Consider having a mode for clean that parses the current manifest (if available)
	 * and keeping all the staged files of the current version. This helps recovering the
	 * current version. Or do it for the previous version to allow a rollback. */

	ret = clean_statedir(options.dry_run, options.all);

	/* TODO: Also print the bytes removed, need to take into account the hardlinks. */
	prettify_size(stats.bytes_removed, &bytes_removed_pretty);
	if (options.dry_run) {
		info("Would remove %d files\n", stats.files_removed);
		info("Aproximatelly %s would be freed\n", bytes_removed_pretty);
	} else {
		info("%d files removed\n", stats.files_removed);
		info("%s freed\n", bytes_removed_pretty);
	}
	FREE(bytes_removed_pretty);

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}

/* clean_statedir will clean the state directory used by swupd (default to
 * /var/lib/swupd). It will remove all files except relevant manifests unless
 * all is set to true. Setting dry_run to true will print the files that would
 * be removed but will not actually remove them. */
enum swupd_code clean_statedir(bool dry_run, bool all)
{
	enum swupd_code ret;
	char *staged_dir = NULL;

	if (!all) {
		if (clock_gettime(CLOCK_REALTIME, &now)) {
			error("couldn't read current time to decide what files to clean\n\n");
			return SWUPD_TIME_UNKNOWN;
		}
	}

	staged_dir = sys_path_join("%s/%s", globals.state_dir, "staged");
	ret = remove_if(staged_dir, dry_run, is_fullfile);
	FREE(staged_dir);
	if (ret != SWUPD_OK) {
		return ret;
	}

	/* Pack presence indicator files. */
	ret = remove_if(globals.state_dir, dry_run, is_pack_indicator);
	if (ret != SWUPD_OK) {
		return ret;
	}

	/* Manifest delta files. */
	ret = remove_if(globals.state_dir, dry_run, is_manifest_delta);
	if (ret != SWUPD_OK) {
		return ret;
	}

	/* NOTE: do not clean the state_dir/bundles directory */
	return clean_staged_manifests(globals.state_dir, dry_run, all);
}
