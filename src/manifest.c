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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd-build-variant.h"
#include "swupd.h"
#include "xattrs.h"

#define MANIFEST_LINE_MAXLEN 8192

#if 0
static int file_sort_hash(const void* a, const void* b)
{
	struct file *A, *B;
	A = (struct file *) a;
	B = (struct file *) b;

	/* this MUST be a memcmp(), not bcmp() */
	return memcmp(A->hash, B->hash, SWUPD_HASH_LEN-1);
}
#endif

/* sort by full path filename */
int file_sort_filename(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;
	A = (struct file *)a;
	B = (struct file *)b;

	ret = strcmp(A->filename, B->filename);
	if (ret) {
		return ret;
	}
	if (A->is_deleted > B->is_deleted) {
		return 1;
	}
	if (A->is_deleted < B->is_deleted) {
		return -1;
	}

	return 0;
}

int file_sort_filename_reverse(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;
	A = (struct file *)a;
	B = (struct file *)b;

	ret = strcmp(A->filename, B->filename);

	return -ret;
}

static int file_sort_version(const void *a, const void *b)
{
	struct file *A, *B;
	A = (struct file *)a;
	B = (struct file *)b;

	if (A->last_change < B->last_change) {
		return -1;
	}
	if (A->last_change > B->last_change) {
		return 1;
	}

	return strcmp(A->filename, B->filename);
}

static int file_found_in_manifest(struct manifest *manifest, struct file *searched_file)
{
	struct list *list;
	struct file *file;

	list = list_head(manifest->files);
	while (list) {
		file = list->data;
		list = list->next;

		if (file->is_deleted) {
			continue;
		}
		if (!strcmp(file->filename, searched_file->filename)) {
			return 1;
		}
	}

	return 0;
}

static int file_has_different_hash_in_manifest(struct manifest *manifest, struct file *searched_file)
{
	struct list *list;
	struct file *file;

	list = list_head(manifest->files);
	while (list) {
		file = list->data;
		list = list->next;

		if (file->is_deleted) {
			continue;
		}
		if (!strcmp(file->filename, searched_file->filename) &&
		    !hash_equal(file->hash, searched_file->hash)) {
			return 1;
		}
	}

	return 0;
}

static struct manifest *alloc_manifest(int version, char *component)
{
	struct manifest *manifest;

	manifest = calloc(1, sizeof(struct manifest));
	if (manifest == NULL) {
		abort();
	}

	manifest->version = version;
	manifest->component = strdup(component);

	return manifest;
}

static struct manifest *manifest_from_file(int version, char *component)
{
	FILE *infile;
	char line[MANIFEST_LINE_MAXLEN], *c, *c2;
	int count = 0;
	int deleted = 0;
	struct manifest *manifest;
	struct list *includes = NULL;
	char *filename;
	uint64_t contentsize = 0;
	int manifest_hdr_version;
	int manifest_enc_version;

	string_or_die(&filename, "%s/%i/Manifest.%s", state_dir, version, component);

	infile = fopen(filename, "rb");
	if (infile == NULL) {
		free(filename);
		return NULL;
	}
	free(filename);

	/* line 1: MANIFEST\t<version> */
	line[0] = 0;
	if (fgets(line, MANIFEST_LINE_MAXLEN - 1, infile) == NULL) {
		goto err_close;
	}

	if (strncmp(line, "MANIFEST\t", 9) != 0) {
		goto err_close;
	}

	c = &line[9];
	manifest_enc_version = strtoull(c, NULL, 10);
	if (manifest_enc_version == 0) {
		goto err_close;
	}

	line[0] = 0;
	while (strcmp(line, "\n") != 0) {
		/* read the header */
		line[0] = 0;
		if (fgets(line, MANIFEST_LINE_MAXLEN - 1, infile) == NULL) {
			break;
		}
		c = strchr(line, '\n');
		if (c) {
			*c = 0;
		} else {
			goto err_close;
		}

		if (strlen(line) == 0) {
			break;
		}
		c = strchr(line, '\t');
		if (c) {
			c++;
		} else {
			goto err_close;
		}

		if (strncmp(line, "version:", 8) == 0) {
			manifest_hdr_version = strtoull(c, NULL, 10);
			if (manifest_hdr_version != version) {
				goto err_close;
			}
		}
		if (strncmp(line, "contentsize:", 12) == 0) {
			contentsize = strtoull(c, NULL, 10);
		}
		if (strncmp(line, "includes:", 9) == 0) {
			includes = list_prepend_data(includes, strdup(c));
			if (!includes->data) {
				abort();
			}
		}
	}

	manifest = alloc_manifest(version, component);
	if (manifest == NULL) {
		goto err_close;
	}

	manifest->contentsize = contentsize;
	manifest->manifest_version = manifest_enc_version;
	manifest->includes = includes;

	/* empty line */
	while (!feof(infile)) {
		struct file *file;

		line[0] = 0;
		if (fgets(line, MANIFEST_LINE_MAXLEN - 1, infile) == NULL) {
			break;
		}
		c = strchr(line, '\n');
		if (c) {
			*c = 0;
		}
		if (strlen(line) == 0) {
			break;
		}

		file = calloc(1, sizeof(struct file));
		if (file == NULL) {
			abort();
		}
		c = line;

		c2 = strchr(c, '\t');
		if (c2) {
			*c2 = 0;
			c2++;
		}

		if (c[0] == 'F') {
			file->is_file = 1;
		} else if (c[0] == 'D') {
			file->is_dir = 1;
		} else if (c[0] == 'L') {
			file->is_link = 1;
		} else if (c[0] == 'M') {
			file->is_manifest = 1;
		} else if (c[0] != '.') { /* unknown file type */
			free(file);
			goto err;
		}

		if (c[1] == 'd') {
			file->is_deleted = 1;
			deleted++;
		} else if (c[1] != '.') { /* unknown modifier #1 */
			free(file);
			goto err;
		}

		if (c[2] == 'C') {
			file->is_config = 1;
		} else if (c[2] == 's') {
			file->is_state = 1;
		} else if (c[2] == 'b') {
			file->is_boot = 1;
		} else if (c[2] != '.') { /* unknown modifier #2 */
			free(file);
			goto err;
		}

		if (c[3] == 'r') {
			file->is_rename = 1;
		} else if (c[3] != '.') { /* unknown modifier #3 */
			free(file);
			goto err;
		}

		c = c2;
		if (!c) {
			free(file);
			continue;
		}
		c2 = strchr(c, '\t');
		if (c2) {
			*c2 = 0;
			c2++;
		} else {
			free(file);
			goto err;
		}

		hash_assign(c, file->hash);

		c = c2;
		c2 = strchr(c, '\t');
		if (c2) {
			*c2 = 0;
			c2++;
		} else {
			free(file);
			goto err;
		}

		file->last_change = strtoull(c, NULL, 10);

		c = c2;

		file->filename = strdup(c);
		if (!file->filename) {
			abort();
		}

		if (file->is_manifest) {
			manifest->manifests = list_prepend_data(manifest->manifests, file);
		} else {
			file->is_tracked = 1;
			manifest->files = list_prepend_data(manifest->files, file);
		}
		count++;
	}

	fclose(infile);
	return manifest;
err:
	free_manifest(manifest);
err_close:
	fclose(infile);
	return NULL;
}

#if 0
/* Print filenames contained in manifest m */
void print_manifest_filenames(struct manifest *m)
{
	struct list *iter = NULL;
	struct file *file = NULL;
	int count = 0;

	iter = list_head(m->files);
	while (iter) {
		count++;
		file = iter->data;
		iter = iter->next;
		printf("%s\n", file->filename);
	}
	printf("Total: %i files\n", count);
}
#endif

void free_manifest_data(void *data)
{
	struct manifest *manifest = (struct manifest *)data;

	free_manifest(manifest);
}

void free_manifest(struct manifest *manifest)
{
	if (!manifest) {
		return;
	}

	if (manifest->manifests) {
		list_free_list_and_data(manifest->manifests, free_file_data);
	}
	if (manifest->submanifests) {
		list_free_list(manifest->files);
		list_free_list_and_data(manifest->submanifests, free_manifest_data);
	} else {
		list_free_list_and_data(manifest->files, free_file_data);
	}
	if (manifest->includes) {
		list_free_list_and_data(manifest->includes, free);
	}
	free(manifest->component);
	free(manifest);
}

static int try_delta_manifest_download(int current, int new, char *component, struct file *file)
{
	char *original = NULL;
	char *newfile = NULL;
	char *deltafile = NULL;
	char *url = NULL;
	int ret = 0;
	struct stat buf;

	if (strcmp(component, "MoM") == 0) {
		// We don't do MoM deltas.
		return -1;
	}

	if (!file->peer) {
		goto out;
	}

	string_or_die(&original, "%s/%i/Manifest.%s", state_dir, current, component);

	populate_file_struct(file, original);
	ret = compute_hash(file, original);
	if (ret != 0) {
		goto out;
	}
	if (!hash_equal(file->peer->hash, file->hash)) {
		goto out;
	}

	string_or_die(&deltafile, "%s/Manifest-%s-delta-from-%i-to-%i", state_dir, component, current, new);

	memset(&buf, 0, sizeof(struct stat));
	ret = stat(deltafile, &buf);
	if (ret || buf.st_size == 0) {
		string_or_die(&url, "%s/%i/Manifest-%s-delta-from-%i", content_url, new, component, current);

		ret = swupd_curl_get_file(url, deltafile, NULL, NULL, false);
		if (ret != 0) {
			unlink(deltafile);
			goto out;
		}
	}

	/* Now apply the manifest delta */
	string_or_die(&newfile, "%s/%i/Manifest.%s", state_dir, new, component);

	ret = apply_bsdiff_delta(original, newfile, deltafile);
	xattrs_copy(original, newfile);
	if (ret != 0) {
		unlink(newfile);
	} else if ((ret = xattrs_compare(original, newfile)) != 0) {
		unlink(newfile);
	}

	unlink(deltafile);

out:
	free(original);
	free(url);
	free(newfile);
	free(deltafile);

	return ret;
}

/* TODO: This should deal with nested manifests better */
static int retrieve_manifests(int current, int version, char *component, struct file *file)
{
	char *url = NULL;
	char *filename;
	char *dir;
	int ret = 0;
	char *tar;
	struct stat sb;

	string_or_die(&filename, "%s/%i/Manifest.%s.tar", state_dir, version, component);
	if (stat(filename, &sb) == 0) {
		ret = 0;
		goto out;
	}

	if (!check_network()) {
		ret = -ENOSWUPDSERVER;
		goto out;
	}

	string_or_die(&dir, "%s/%i", state_dir, version);
	ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if ((ret != 0) && (errno != EEXIST)) {
		goto out;
	}
	free(dir);

	/* FILE is not set for a MoM, only for bundle manifests */
	if (file && current < version) {
		if (try_delta_manifest_download(current, version, component, file) == 0) {
			return 0;
		}
	}

	string_or_die(&url, "%s/%i/Manifest.%s.tar", content_url, version, component);

	ret = swupd_curl_get_file(url, filename, NULL, NULL, false);
	if (ret) {
		unlink(filename);
		goto out;
	}

	string_or_die(&tar, TAR_COMMAND " -C %s/%i -xf %s/%i/Manifest.%s.tar 2> /dev/null",
		      state_dir, version, state_dir, version, component);

	/* this is is historically a point of odd errors */
	ret = system(tar);
	if (WIFEXITED(ret)) {
		ret = WEXITSTATUS(ret);
	}
	free(tar);
	if (ret != 0) {
		goto out;
	}

out:
	free(filename);
	free(url);
	return ret;
}

static void set_untracked_manifest_files(struct manifest *manifest)
{
	struct list *files;

	if (!manifest || is_tracked_bundle(manifest->component)) {
		return;
	}

	files = manifest->files;

	while (files) {
		struct file *file = files->data;
		file->is_tracked = 0;
		files = files->next;
	}
}

/* Loads the MoM (Manifest of Manifests) for VERSION.
 *
 * Implementation note: MoMs are not huge so deltas do not give much benefit,
 * versus bundle manifests which can be huge and big for deltas. If there was a
 * MoM delta you would want to validate the input MoM prior to applying a
 * delta, and then validate the output MoM. All complicated compared to just
 * getting the whole MoM and verifying it.
 *
 * Note that if the manifest fails to download, or if the manifest fails to be
 * loaded into memory, this function will return NULL.
 */
struct manifest *load_mom(int version)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	char *filename;
	char *url;
	char *log_cmd = NULL;

	ret = retrieve_manifests(version, version, "MoM", NULL);
	if (ret != 0) {
		printf("Failed to retrieve %d MoM manifest\n", version);
		return NULL;
	}

	string_or_die(&filename, "%s/%i/Manifest.MoM", state_dir, version);
	string_or_die(&url, "%s/%i/Manifest.MoM", content_url, version);
	if (!download_and_verify_signature(url, filename)) {
		if (sigcheck) {
			printf("WARNING!!! FAILED TO VERIFY SIGNATURE OF Manifest.MoM\n");
			free(filename);
			free(url);
			return NULL;
		} else {
			printf("FAILED TO VERIFY SIGNATURE OF Manifest.MoM. Operation proceeding due to\n"
				"  --nosigcheck, but system security may be compromised\n");
			string_or_die(&log_cmd, "echo \"swupd security notice:"
				" --nosigcheck used to bypass MoM signature verification failure\" | systemd-cat");
			(void)system(log_cmd);
			free(log_cmd);
		}
	}
	free(filename);
	free(url);

	manifest = manifest_from_file(version, "MoM");

	if (manifest == NULL) {
		printf("Failed to load %d MoM manifest\n", version);
		goto out;
	}

	return manifest;

out:
	return NULL;
}

/* Loads the MANIFEST for bundle associated with FILE at VERSION, referenced by
 * the given MOM manifest.
 *
 * Value ranges and restrictions:
 *   - CURRENT < VERSION for 'update' subcommand
 *   - CURRENT == VERSION for other subcommands
 *
 * The FILENAME member of FILE contains the name of the bundle. Also, the HASH
 * member of FILE is needed for delta manifest application. The values of
 * CURRENT and VERSION are used to determine which delta manifest to apply.
 *
 * Note that if the manifest fails to download, or if the manifest fails to be
 * loaded into memory, this function will return NULL.
 */
struct manifest *load_manifest(int current, int version, struct file *file, struct manifest *mom)
{
	struct manifest *manifest = NULL;
	int ret = 0;

	ret = retrieve_manifests(current, version, file->filename, file);
	if (ret != 0) {
		printf("Failed to retrieve %d %s manifest\n", version, file->filename);
		return NULL;
	}

	ret = verify_bundle_hash(mom, file);
	if (ret != 0) {
		return NULL;
	}

	manifest = manifest_from_file(version, file->filename);

	if (manifest == NULL) {
		printf("Failed to load %d %s manifest\n", version, file->filename);
		return NULL;
	}

	set_untracked_manifest_files(manifest);

	return manifest;
}

/* Find files which need updated based on deltas in last_change.
   Should let further do_not_update policy be handled in the caller, but for
   now some hacky exclusions are done here. */
struct list *create_update_list(struct manifest *current, struct manifest *server)
{
	struct list *output = NULL;
	struct list *list;
	char *fullname = NULL;

	update_count = 0;
	update_skip = 0;
	list = list_head(server->files);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;

		if (file->peer && hash_equal(file->hash, file->peer->hash)) {
			/* Skip new/changed files if they are already installed
			 * and the hash check passes. This makes minversion
			 * bumps cheaper. */
			fullname = mk_full_filename(path_prefix, file->filename);
			if (fullname == NULL) {
				abort();
			}
			if (verify_file(file, fullname)) {
				free(fullname);
				continue;
			}
			free(fullname);
		}

		if ((file->last_change > current->version) ||
		    (file->is_rename && file_has_different_hash_in_manifest(current, file)) ||
		    !file->is_tracked) {
			/* check and if needed mark as do_not_update */
			(void)ignore(file);
			/* check if we need to run scripts/update the bootloader/etc */
			apply_heuristics(file);

			output = list_prepend_data(output, file);
		}
	}
	update_count = list_len(output) - update_skip;

	return output;
}

/* m1: old (or current when verifying) manifest
 * m2: new (or official if verifying) manifest */
void link_manifests(struct manifest *m1, struct manifest *m2)
{
	struct list *list1, *list2;
	struct file *file1, *file2;

	m1->files = list_sort(m1->files, file_sort_filename);
	m2->files = list_sort(m2->files, file_sort_filename);

	list1 = list_head(m1->files);
	list2 = list_head(m2->files);

	while (list1 && list2) { /* m1/file1 matches m2/file2 */
		int ret;
		file1 = list1->data;
		file2 = list2->data;

		ret = strcmp(file1->filename, file2->filename);
		if (ret == 0) {
			if (!file1->is_deleted || file2->is_deleted) {
				file1->peer = file2;
				file2->peer = file1;
				file1->deltapeer = file2;
				file2->deltapeer = file1;
			}

			/* When new bundle is added to the include list of an installed bundle,
				we must use the more recent, tracked last_change to ensure new files
				are not assumed already present */
			if (!file1->is_deleted && !file2->is_deleted
					&& hash_equal(file1->hash, file2->hash)
					&& file1->is_tracked) {
				file2->last_change = file1->last_change;
			}

			list1 = list1->next;
			list2 = list2->next;

			if (((file2->last_change == m2->version) || (file2->last_change > m1->version)) && !file2->is_deleted) {
				account_changed_file();
			}
			if (file2->last_change > m1->version && file2->is_deleted) {
				account_deleted_file();
			}
			continue;
		}
		if (ret < 0) { /*  m1/file1 is before m2/file2 */
			/* File is absent in m2, indicating that a format bump
			 * happened, removing deleted file entries from the
			 * previous format(s). Do not account the deleted file
			 * in this case, since an update will not delete the
			 * file.
			 */
			list1 = list1->next;
			continue;
		} /* else ret > 0  m1/file1 is after m2/file2 */
		list2 = list2->next;
		account_new_file();
	}
}

/* m1: old manifest
 * m2: new manifest */
void link_submanifests(struct manifest *m1, struct manifest *m2)
{
	struct list *list1, *list2;
	struct file *file1, *file2;

	m1->manifests = list_sort(m1->manifests, file_sort_filename);
	m2->manifests = list_sort(m2->manifests, file_sort_filename);

	list1 = list_head(m1->manifests);
	list2 = list_head(m2->manifests);

	while (list1 && list2) { /* m1/file1 matches m2/file2 */
		int ret;
		file1 = list1->data;
		file2 = list2->data;

		ret = strcmp(file1->filename, file2->filename);
		if (ret == 0) {
			file1->peer = file2;
			file2->peer = file1;
			file1->deltapeer = file2;
			file2->deltapeer = file1;
			list1 = list1->next;
			list2 = list2->next;

			if (((file2->last_change == m2->version) || (file2->last_change > m1->version)) && !file2->is_deleted) {
				account_changed_manifest();
			}
			if (file2->last_change == m2->version && file2->is_deleted) {
				account_deleted_manifest();
			}
			continue;
		}
		if (ret < 0) { /*  m1/file1 is before m2/file2 */
			list1 = list1->next;
			account_deleted_manifest();
			continue;
		} /* else ret > 0  m1/file1 is after m2/file2 */
		list2 = list2->next;
		account_new_manifest();
	}
}

/* if component is specified explicitly, pull in submanifest only for that
 * if component is not specified, pull in any tracked component submanifest */
struct list *recurse_manifest(struct manifest *manifest, const char *component)
{
	struct list *bundles = NULL;
	struct list *list;
	struct file *file;
	struct manifest *sub;
	int version1, version2;

	manifest->contentsize = 0;
	list = manifest->manifests;
	while (list) {
		file = list->data;
		list = list->next;

		if (!component && !component_subscribed(file->filename)) {
			continue;
		}

		if (component && !(strcmp(component, file->filename) == 0)) {
			continue;
		}

		version2 = file->last_change;
		if (file->peer) {
			version1 = file->peer->last_change;
		} else {
			version1 = version2;
		}
		if (version1 > version2) {
			version1 = version2;
		}

		sub = load_manifest(version1, version2, file, manifest);
		if (!sub) {
			list_free_list_and_data(bundles, free_manifest_data);
			return NULL;
		}
		if (sub != NULL) {
			bundles = list_prepend_data(bundles, sub);
		}
	}

	return bundles;
}

struct list *consolidate_files(struct list *files)
{
	struct list *list, *next, *tmp;
	struct file *file1, *file2;

	files = list_sort(files, file_sort_filename);

	/* Two pointers ("list" and "next") traverse the consolidated, filename sorted
	 * struct list of files.  The "list" pointer is marched forward through the
	 * struct list as long as it and the next do not point
	 * to two objects with the same filename.  If the name is the same, then
	 * "list" and "next" point to the first and second in a series of perhaps
	 * many objects referring to the same filename.  As we determine which file out
	 * of multiples to keep in our consolidated, deduplicated, filename sorted list
	 * there are Manifest invariants to maintain.  The following table shows the
	 * associated decision matrix.  Note that "file" may be a file, directory or
	 * symlink.
	 *
	 *         | File 2:
	 *         |  A'    B'    C'    D'
	 * File 1: |------------------------
	 *    A    |  -  |  2  |  2  |  2  |
	 *    B    |  1  |  -  |  2  |  2  |
	 *    C    |  1  |  1  |  -  |  X  |
	 *    D    |  1  |  1  |  X  |  X  |
	 *
	 *   State for file1 {A,B,C,D}
	 *         for file2 {A',B',C',D'}
	 *       A:  is_deleted && !is_rename
	 *       B:  is_deleted &&  is_rename
	 *       C: !is_deleted && (file1->hash == file2->hash)
	 *       D: !is_deleted && (file1->hash != file2->hash)
	 *
	 *   Action
	 *       -: Don't Care   - choose/remove either file
	 *       X: Error State  - remove both files, LOG error
	 *       1: choose file1 - remove file2
	 *       2: choose file2 - remove file1
	 *
	 * NOTE: the code below could be rewritten to be more "efficient", but clarity
	 *       and concreteness here are of utmost importance if we are to correctly
	 *       maintain the installed system's state in the filesystem across updates
	 */
	list = list_head(files);
	while (list) {
		next = list->next;
		if (next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = next->data;

		if (strcmp(file1->filename, file2->filename)) {
			list = next;
			continue;
		} /* from here on, file1 and file2 have a filename match */

		/* (case 1) C and C'               : choose file1
		 * this is the most common case, so test it first */
		if (!file1->is_deleted && !file2->is_deleted && hash_equal(file1->hash, file2->hash)) {
			/* always drop the untracked file if there is a tracked file */
			if (file1->is_tracked && !file2->is_tracked) {
				list_free_item(next, NULL);
			} else if (!file1->is_tracked && file2->is_tracked) {
				list_free_item(list, NULL);
				list = next;
			} else if (file1->last_change <= file2->last_change) { /* drop the newer of the two */
				list_free_item(next, NULL);
			} else {
				list_free_item(list, NULL);
				list = next;
			}
			continue;
		}

		if (file1->is_deleted && file2->is_deleted) {
			/* keep the newer of the two */
			if (file1->last_change > file2->last_change) {
				list_free_item(next, NULL);
			} else {
				list_free_item(list, NULL);
				list = next;
			}
			continue;
		}

		/* (case 2) A'                     : choose file1 */
		if (file2->is_deleted && !file2->is_rename) {
			list_free_item(next, NULL);
			continue;
		}
		/* (case 3) A                      : choose file2 */
		if (file1->is_deleted && !file1->is_rename) {
			list_free_item(list, NULL);
			list = next;
			continue;
		}
		/* (case 4) B' AND NOT A           : choose file 1*/
		if (file2->is_deleted && file2->is_rename) { // && !(file1->is_deleted && !file1->is_rename)
			list_free_item(next, NULL);
			continue;
		}

		/* (case 5) B AND NOT (A' OR B')   : choose file2 */
		if (file1->is_deleted && file1->is_rename) { // && !(file2->is_deleted)
			list_free_item(list, NULL);
			list = next;
			continue;
		}

		/* (case 6) all others constitute errors */
		tmp = next->next;
		list_free_item(list, NULL);
		list_free_item(next, NULL);
		list = tmp;
	}

	return list;
}

#if 0
/* non-production code: a useful function to have and maintain and some useful
 * call sites exist though they are commented out */
static char *type_to_string(struct file *file)
{
	static char type[5];

	strcpy(type, "....");

	if (file->is_dir) {
		type[0] = 'D';
	}

	if (file->is_link) {
		type[0] = 'L';
	}
	if (file->is_file) {
		type[0] = 'F';
	}
	if (file->is_manifest) {
		type[0] = 'M';
	}

	if (file->is_deleted) {
		type[1] = 'd';
	}

	if (file->is_config) {
		type[2] = 'C';
	}
	if (file->is_state) {
		type[2] = 's';
	}
	if (file->is_boot) {
		type[2] = 'b';
	}

	if (file->is_rename) {
		type[3] = 'r';
	}

	return type;
}

void debug_write_manifest(struct manifest *manifest, char *filename)
{
	struct list *list;
	struct file *file;
	FILE *out;
	char *fullfile = NULL;

	string_or_die(&fullfile, "%s/%s", state_dir, filename);

	out = fopen(fullfile, "w");
	if (out == NULL) {
		printf("Failed to open %s for write\n", fullfile);
	}
	if (out == NULL) {
		abort();
	}

	fprintf(out, "MANIFEST\t1\n");
	fprintf(out, "version:\t%i\n", manifest->version);

	list = list_head(manifest->files);
	fprintf(out, "\n");
	while (list) {
		file = list->data;
		list = list->next;
		fprintf(out, "%s\t%s\t%i\t%s\n", type_to_string(file), file->hash, file->last_change, file->filename);
	}


	list = list_head(manifest->manifests);
	while (list) {
		file = list->data;
		list = list->next;
		fprintf(out, "%s\t%s\t%i\t%s\n", type_to_string(file), file->hash, file->last_change, file->filename);
	}
	fclose(out);
	free(fullfile);
}
#endif

void link_renames(struct list *newfiles, struct manifest *from_manifest)
{
	struct list *list1, *list2;
	struct list *targets;
	struct file *file1, *file2;

	targets = newfiles = list_sort(newfiles, file_sort_version);

	list1 = list_head(newfiles);

	/* todo: sort newfiles and targets by hash */

	while (list1) {
		file1 = list1->data;
		list1 = list1->next;

		if (file1->peer || !file1->is_rename) {
			continue;
		}
		if (file1->is_deleted) {
			continue;
		}
		/* now, file1 is the new file that got renamed. time to search the rename targets */
		list2 = list_head(targets);
		while (list2) {
			file2 = list2->data;
			list2 = list2->next;

			if (!file2->peer || !file2->is_rename) {
				continue;
			}
			if (!file2->is_deleted) {
				continue;
			}
			if (!file_found_in_manifest(from_manifest, file2)) {
				continue;
			}
			if (hash_equal(file2->hash, file1->hash)) {
				file1->deltapeer = file2->peer;
				file1->peer = file2->peer;
				file2->peer->deltapeer = file1;
				list2 = NULL;
			}
		}
	}
}

void populate_file_struct(struct file *file, char *filename)
{
	struct stat stat;

	memset(&stat, 0, sizeof(stat));
	if (lstat(filename, &stat) < 0) {
		if (errno == ENOENT) {
			file->is_deleted = 1;
			return;
		}
		return;
	}

	file->is_deleted = 0;

	file->stat.st_mode = stat.st_mode;
	file->stat.st_uid = stat.st_uid;
	file->stat.st_gid = stat.st_gid;
	file->stat.st_rdev = stat.st_rdev;
	file->stat.st_size = stat.st_size;

	if (S_ISLNK(stat.st_mode)) {
		file->is_file = 0;
		file->is_dir = 0;
		file->is_link = 1;
		memset(&file->stat.st_mode, 0, sizeof(file->stat.st_mode));
		return;
	}

	if (S_ISDIR(stat.st_mode)) {
		file->is_file = 0;
		file->is_dir = 1;
		file->is_link = 0;
		file->stat.st_size = 0;
		return;
	}

	/* if we get here, this is a regular file */
	file->is_file = 1;
	file->is_dir = 0;
	file->is_link = 0;

#if 0
	dump_file_info(file);
#endif
	return;
}

/* Iterate m file list and remove from filesystem each file/directory */
void remove_files_in_manifest_from_fs(struct manifest *m)
{
	struct list *iter = NULL;
	struct file *file = NULL;
	char *fullfile = NULL;
	int count = 0;

	iter = list_head(m->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		string_or_die(&fullfile, "%s/%s", path_prefix, file->filename);
		if (swupd_rm(fullfile) != 0) {
			free(fullfile);
			continue;
		}
		free(fullfile);
		count++;
	}
	printf("Total deleted files: %i\n", count);
}

/* free all files found in m1 that happens to be
*  duplicated on m2.
*
*  Is _mandatory_ to have both manifests files lists ordered by filename before
*  calling this function.
*
*  This function frees (removes) all duplicated files matching those in m2 from
*  m1, the resulting m1 file list will be unique respect to files
*  contained on m2.
*
*  Although we can generate a third manifest containing the unique file list
*  to be more simple in code, this approach pops out and free each duplicated
*  from m1 in place, this means a gain on space and speed, since it does not
*  need to allocate new manifest file list, and since it frees duplicates here,
*  it does not need to free more elements later.
*/
void deduplicate_files_from_manifest(struct manifest **m1, struct manifest *m2)
{
	struct list *iter1, *iter2, *cur_file, *preserver = NULL;
	struct file *file1, *file2 = NULL;
	struct manifest *bmanifest = NULL;
	int ret;
	int count = 0;

	bmanifest = *m1;
	iter1 = preserver = list_head(bmanifest->files);
	iter2 = list_head(m2->files);

	while (iter1 && iter2) {
		file1 = iter1->data;
		file2 = iter2->data;
		cur_file = iter1;

		ret = strcmp(file1->filename, file2->filename);
		if (ret == 0) {
			iter1 = iter1->next;
			iter2 = iter2->next;

			/* file is deleted initially and hence keep in list to remove */
			if (!file1->is_deleted && file2->is_deleted) {
				continue;
			}
			preserver = free_list_file(cur_file);
			count++;
		} else if (ret < 0) {
			iter1 = iter1->next;
			/* file already deleted, pull it out of the list */
			if (file1->is_deleted) {
				preserver = free_list_file(cur_file);
				count++;
			}
		} else {
			iter2 = iter2->next;
		}
	}

	/* give me back my list pointer */
	bmanifest->files = preserver;
}

struct file *search_bundle_in_manifest(struct manifest *manifest, const char *bundlename)
{
	struct list *iter = NULL;
	struct file *file;

	iter = list_head(manifest->manifests);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		if (strcmp(file->filename, bundlename) == 0) {
			return file;
		}
	}

	return NULL;
}

/* This performs a linear search through the files list. */
struct file *search_file_in_manifest(struct manifest *manifest, const char *filename)
{
	struct list *iter = NULL;
	struct file *file;

	iter = list_head(manifest->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (strcmp(file->filename, filename) == 0) {
			return file;
		}
	}
	return NULL;
}
