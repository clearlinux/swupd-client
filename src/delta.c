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
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <bsdiff.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"
#include "xattrs.h"

static bool compute_hash_from_file(char *filename, char *hash)
{
	/* TODO: implement this without the indirection of creating a file... */
	struct file f = { 0 };

	f.filename = filename;
	f.use_xattrs = true;

	populate_file_struct(&f, filename);
	if (compute_hash(&f, filename) != 0) {
		return false;
	}

	hash_assign(&f.hash[0], hash);
	return true;
}

static void apply_one_delta(char *from_file, char *to_staged, char *delta_file, char *to_hash)
{
	int ret = apply_bsdiff_delta(from_file, to_staged, delta_file);
	if (ret) {
		return;
	}

	xattrs_copy(from_file, to_staged);

	char hash[SWUPD_HASH_LEN];
	if (!compute_hash_from_file(to_staged, &hash[0])) {
		warn("Couldn't use delta file %s: hash calculation failed\n", delta_file);
		(void)remove(to_staged);
		return;
	}
	if (!hash_equal(hash, to_hash)) {
		warn("Couldn't use delta file %s: application resulted in wrong hash\n", delta_file);
		(void)remove(to_staged);
	}
}

/* Check if the delta filename is well-formed, if so return true and fill the from/to
 * buffers with the corresponding hashes. Return false otherwise. */
static bool check_delta_filename(const char *delta_name, char *from, char *to)
{
	/* Delta files have the form [FROM_VERSION]-[TO_VERSION]-[FROM_HASH]-[TO_HASH]. */
	const char *s = delta_name;

	/* Ignore versions, deltas will be used based on their hashes only. */
	for (int i = 0; i < 2; i++) {
		s = strchr(s, '-');
		if (!s) {
			return false;
		}
		s++;
	}

	/* Note: SWUPD_HASH_LEN accounts for the NUL-terminator after the hash. */
	const size_t hash_len = SWUPD_HASH_LEN - 1;
	if (string_len(s) != (hash_len * 2 + 1)) {
		return false;
	}

	if (s[hash_len] != '-') {
		return false;
	}

	hash_assign(s, from);

	/* Consume the first hash and the separator. */
	s += hash_len + 1;

	hash_assign(s, to);

	return true;
}

void apply_deltas(struct manifest *current_manifest)
{
	char *delta_dir;
	string_or_die(&delta_dir, "%s/delta", globals.state_dir);

	DIR *dir = opendir(delta_dir);
	if (!dir) {
		/* No deltas available to apply. */
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}

		char *to_staged = NULL;

		char *delta_name = ent->d_name;
		char *delta_file;
		string_or_die(&delta_file, "%s/%s", delta_dir, delta_name);

		char from[SWUPD_HASH_LEN] = { 0 };
		char to[SWUPD_HASH_LEN] = { 0 };
		if (!check_delta_filename(delta_name, &from[0], &to[0])) {
			warn("Invalid name for delta file: %s\n", delta_file);
			goto next;
		}

		string_or_die(&to_staged, "%s/staged/%s", globals.state_dir, to);

		/* If 'to' file already exists, no need to apply delta. */
		struct stat stat;
		if (lstat(to_staged, &stat) == 0) {
			goto next;
		}

		/* TODO: Sort the list of files by hash then walk it
		 * in parallel with the (sorted) results of readdir. */

		struct list *ll = list_head(current_manifest->files);
		char *found = NULL;
		bool bad_on_sys = false;

		for (; ll && !found; ll = ll->next) {
			struct file *file = ll->data;
			if (file->is_deleted || file->is_ghosted || !file->is_file || !hash_equal(file->hash, from)) {
				continue;
			}

			/* Verify the actual file in the disk matches our expectations. */
			char hash[SWUPD_HASH_LEN];
			char *filename;
			string_or_die(&filename, "%s/%s", globals.path_prefix, file->filename);

			if (!compute_hash_from_file(filename, hash) || !hash_equal(file->hash, hash)) {
				free_and_clear_pointer(&filename);
				bad_on_sys = true;
				continue;
			}

			found = filename;
		}

		if (!found) {
			warn("Couldn't use delta file %s: ", delta_file);
			if (bad_on_sys) {
				info("'from' file corrupted on system, consider running 'swupd verify --fix'\n");
			} else {
				info("no 'from' file to apply was found\n");
			}
			goto next;
		}

		apply_one_delta(found, to_staged, delta_file, to);
		free_and_clear_pointer(&found);

	next:
		/* Always remove delta files. Once applied the full staged file will be
		 * available, so no need to keep the delta around. */
		sys_rm(delta_file);
		free_and_clear_pointer(&delta_file);
		free_and_clear_pointer(&to_staged);
	}

	closedir(dir);
	free_and_clear_pointer(&delta_dir);
}
