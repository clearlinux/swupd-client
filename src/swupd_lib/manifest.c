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
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#include <bsdiff.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "signature.h"
#include "swupd.h"
#include "swupd_build_variant.h"
#include "xattrs.h"

#define MANIFEST_LINE_MAXLEN 8192

static struct manifest *manifest_from_file(int version, char *component, bool header_only)
{
	char *filename;
	struct manifest *manifest;

	filename = statedir_get_manifest(version, component);
	manifest = manifest_parse(component, filename, header_only);
	FREE(filename);

	if (!manifest) {
		return NULL;
	}

	if (manifest->version != version) {
		error("Loaded incompatible manifest header version for %s: %d != %d\n", component, manifest->version, version);
		manifest_free(manifest);

		return NULL;
	}

	return manifest;
}

static int try_manifest_delta_download(int from, int to, char *component)
{
	char *from_manifest = NULL;
	char *to_manifest = NULL;
	char *manifest_delta = NULL;
	char *to_dir = NULL;
	char *url = NULL;
	int ret = 0;

	if (from <= 0 || from > to) {
		// We don't have deltas for going back
		return -1;
	}

	if (str_cmp(component, "MoM") == 0) {
		// We don't do MoM deltas.
		return -1;
	}

	if (str_cmp(component, "full") == 0) {
		// We don't do full manifest deltas.
		return -1;
	}

	from_manifest = statedir_get_manifest(from, component);
	to_manifest = statedir_get_manifest(to, component);

	manifest_delta = statedir_get_manifest_delta(component, from, to);
	to_dir = statedir_get_manifest_dir(to);

	if (!sys_file_exists(manifest_delta)) {
		string_or_die(&url, "%s/%i/Manifest-%s-delta-from-%i", globals.content_url, to, component, from);
		ret = swupd_curl_get_file(url, manifest_delta);
		FREE(url);
		if (ret != 0) {
			unlink(manifest_delta);
			goto out;
		}
	}

	/* Now apply the manifest delta */

	mkdir_p(to_dir); // Create destination dir if necessary
	ret = apply_bsdiff_delta(from_manifest, to_manifest, manifest_delta);
	if (ret != 0) {
		unlink(to_manifest);
		goto out;
	}

	xattrs_copy(to_manifest, from_manifest);
	if ((ret = xattrs_compare(from_manifest, to_manifest)) != 0) {
		unlink(to_manifest);
		goto out;
	}

out:
	FREE(to_manifest);
	FREE(to_dir);
	FREE(from_manifest);
	FREE(manifest_delta);
	return ret;
}

/* TODO: This should deal with nested manifests better */
static int retrieve_manifest(int previous_version, int version, char *component, bool retry)
{
	char *url = NULL;
	char *filename;
	char *filename_cache = NULL;
	char *dir = NULL;
	int ret = 0;

	/* Check for fullfile only, we will not be keeping the .tar around */
	filename = statedir_get_manifest(version, component);
	if (sys_file_exists(filename)) {
		ret = 0;
		goto out;
	}

	dir = statedir_get_manifest_dir(version);
	ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if ((ret != 0) && (errno != EEXIST)) {
		goto out;
	}

	/* Check statedir-cache */
	if (globals.state_dir_cache != NULL) {
		filename_cache = statedir_dup_get_manifest(version, component);
		if (link_or_copy(filename_cache, filename) == 0) {
			ret = 0;
			goto out;
		}
	}

	if (!retry && try_manifest_delta_download(previous_version, version, component) == 0) {
		ret = 0;
		goto out;
	}
	FREE(filename);

	filename = statedir_get_manifest_tar(version, component);
	string_or_die(&url, "%s/%i/Manifest.%s.tar", globals.content_url, version, component);

	ret = swupd_curl_get_file(url, filename);
	if (ret) {
		unlink(filename);
		goto out;
	}

	ret = archives_extract_to(filename, dir);
	if (ret != 0) {
		goto out;
	} else {
		unlink(filename);
	}

out:
	FREE(dir);
	FREE(filename);
	FREE(filename_cache);
	FREE(url);
	return ret;
}

static void set_untracked_manifest_files(struct manifest *manifest)
{
	struct list *files;

	if (!manifest || is_installed_bundle(manifest->component)) {
		return;
	}

	files = manifest->files;

	while (files) {
		struct file *file = files->data;
		file->is_tracked = 0;
		files = files->next;
	}
}

static void remove_manifest(char *path, char *filename, char *hash)
{
	char *file = NULL;

	file = sys_path_join("%s/Manifest.%s", path, filename);
	unlink(file);
	FREE(file);
	file = sys_path_join("%s/Manifest.%s.tar", path, filename);
	unlink(file);
	FREE(file);
	file = sys_path_join("%s/Manifest.%s.sig", path, filename);
	unlink(file);
	FREE(file);
	if (hash != NULL) {
		file = sys_path_join("%s/Manifest.%s.%s", path, filename, hash);
		unlink(file);
		FREE(file);
	}
}

/* Removes the extracted Manifest.<bundle> and accompanying tar file, cache file, and
 * the signature file from the statedir and statedir-cache */
static void remove_manifest_files(char *filename, int version, char *hash)
{
	char *manifest_dir = NULL;

	warn("Removing corrupt Manifest.%s artifacts and re-downloading...\n", filename);

	// remove the manifest from the statedir first
	manifest_dir = statedir_get_manifest_dir(version);
	remove_manifest(manifest_dir, filename, hash);
	FREE(manifest_dir);

	// if there is a statedir cache (duplicate), then remove it from there too
	if (globals.state_dir_cache) {
		manifest_dir = statedir_dup_get_manifest_dir(version);
		remove_manifest(manifest_dir, filename, hash);
		FREE(manifest_dir);
	}
}

/* Verifies signature for the local file DATA_FILENAME first, and on failure
 * downloads the signature based on DATA_URL and tries to verify again.
 *
 * returns: true if signature verification succeeded, false if verification
 * failed, or the signature download failed
 */
static bool mom_signature_verify(const char *data_url, const char *data_filename, int version)
{
	char *sig_url = NULL;
	char *sig_filename = NULL;
	char *sig_filename_cache = NULL;
	int ret;
	bool result;

	string_or_die(&sig_url, "%s.sig", data_url);
	string_or_die(&sig_filename, "%s.sig", data_filename);

	// Try verifying a local copy of the signature first
	result = signature_verify(data_filename, sig_filename, SIGNATURE_DEFAULT);
	if (result) {
		goto out;
	}

	// Check statedir-cache
	if (globals.state_dir_cache != NULL) {
		sig_filename_cache = statedir_dup_get_manifest(version, "MoM.sig");
		if (link_or_copy(sig_filename_cache, sig_filename) == 0) {
			result = signature_verify(data_filename, sig_filename, SIGNATURE_DEFAULT);

			if (result) {
				goto out;
			}
		}
	}

	ret = swupd_curl_get_file(sig_url, sig_filename);
	if (ret == 0) {
		result = signature_verify(data_filename, sig_filename, SIGNATURE_PRINT_ERRORS);
	} else {
		// download failed
		result = false;
	}
out:
	FREE(sig_filename);
	FREE(sig_url);
	FREE(sig_filename_cache);
	return result;
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
 * loaded into memory, this function will return NULL. If err is passed, it is set
 * with the error code.
 */
struct manifest *load_mom(int version, int *err)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	char *filename;
	char *url;
	bool retried = false;

retry_load:
	ret = retrieve_manifest(0, version, "MoM", retried);
	if (ret != 0) {
		error("Failed to retrieve %d MoM manifest\n", version);
		if (err) {
			*err = ret;
		}
		return NULL;
	}

	manifest = manifest_from_file(version, "MoM", false);

	if (manifest == NULL) {
		if (retried == false) {
			remove_manifest_files("MoM", version, NULL);
			retried = true;
			goto retry_load;
		}
		error("Failed to load %d MoM manifest\n", version);
		if (err) {
			*err = SWUPD_COULDNT_LOAD_MANIFEST;
		}
		return NULL;
	}

	filename = statedir_get_manifest(version, "MoM");
	string_or_die(&url, "%s/%i/Manifest.MoM", globals.content_url, version);

	if (!globals.sigcheck) {
		warn_nosigcheck(url);
	} else {
		/* Only when migrating , ignore the locally made signature check which is guaranteed to have been signed
		 * by the user and does not come from any network source */
		if (!mom_signature_verify(url, filename, version)) {
			/* cleanup and try one more time, statedir could have got corrupt/stale */
			if (retried == false) {
				FREE(filename);
				FREE(url);
				manifest_free(manifest);
				remove_manifest_files("MoM", version, NULL);
				retried = true;
				goto retry_load;
			}
			error("Signature verification failed for manifest version %d\n", version);
			FREE(filename);
			FREE(url);
			manifest_free(manifest);
			if (err) {
				*err = SWUPD_SIGNATURE_VERIFICATION_FAILED;
			}
			return NULL;
		}
	}

	FREE(filename);
	FREE(url);
	return manifest;
}

/* Loads the MANIFEST for bundle associated with FILE at VERSION, referenced by
 * the given MOM manifest.
 *
 * The FILENAME member of FILE contains the name of the bundle.
 *
 * Note that if the manifest fails to download, or if the manifest fails to be
 * loaded into memory, this function will return NULL.
 */
struct manifest *load_manifest(int version, struct file *file, struct manifest *mom, bool header_only, int *err)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	int prev_version;
	bool retried = false;

retry_load:
	prev_version = file->peer ? file->peer->last_change : 0;
	ret = retrieve_manifest(prev_version, version, file->filename, retried);
	if (ret != 0) {
		error("Failed to retrieve %d %s manifest\n", version, file->filename);
		if (err) {
			*err = ret;
		}
		return NULL;
	}

	ret = verify_bundle_hash(mom, file);
	if (ret != 0) {
		if (retried == false) {
			remove_manifest_files(file->filename, version, file->hash);
			retried = true;
			goto retry_load;
		}
		if (err) {
			*err = SWUPD_SIGNATURE_VERIFICATION_FAILED;
		}
		return NULL;
	}

	manifest = manifest_from_file(version, file->filename, header_only);

	if (manifest == NULL) {
		if (retried == false) {
			remove_manifest_files(file->filename, version, file->hash);
			retried = true;
			goto retry_load;
		}
		error("Failed to load %d %s manifest\n", version, file->filename);
		if (err) {
			*err = SWUPD_COULDNT_LOAD_MANIFEST;
		}
		return NULL;
	}

	set_untracked_manifest_files(manifest);

	return manifest;
}

/* m1: old (or current when verifying) manifest
 * m2: new (or official if verifying) manifest */
void link_manifests(struct manifest *m1, struct manifest *m2)
{
	struct list *list1, *list2;
	struct file *file1, *file2;

	m1->files = list_sort(m1->files, cmp_file_filename_is_deleted);
	m2->files = list_sort(m2->files, cmp_file_filename_is_deleted);

	list1 = list_head(m1->files);
	list2 = list_head(m2->files);

	while (list1 && list2) { /* m1/file1 matches m2/file2 */
		int ret;
		file1 = list1->data;
		file2 = list2->data;

		ret = str_cmp(file1->filename, file2->filename);
		if (ret == 0) {
			if (!file1->is_deleted || file2->is_deleted) {
				file1->peer = file2;
				file2->peer = file1;
			}

			list1 = list1->next;
			list2 = list2->next;

			if (file2->last_change > m1->version && !file2->is_deleted) {
				account_changed_file();
			}
			if (file2->last_change > m1->version && file2->is_deleted) {
				account_deleted_file();
			}
			continue;
		}
		if (ret < 0) { /*  m1/file1 is before m2/file2 */
			account_deleted_file();
			list1 = list1->next;
			continue;
		} /* else ret > 0  m1/file1 is after m2/file2 */
		list2 = list2->next;
		account_new_file();
	}

	// Capture deleted files if they are at the tail end of the file list
	while (list1) {
		list1 = list1->next;
		account_deleted_file();
	}

	// Capture new files if they are at the tail end of the file list
	while (list2) {
		list2 = list2->next;
		account_new_file();
	}
}

/* m1: old manifest
 * m2: new manifest */
void link_submanifests(struct manifest *m1, struct manifest *m2, struct list *subs1, struct list *subs2, bool server)
{
	struct list *list1, *list2;
	struct file *file1, *file2;

	m1->manifests = list_sort(m1->manifests, cmp_file_filename_is_deleted);
	m2->manifests = list_sort(m2->manifests, cmp_file_filename_is_deleted);

	list1 = list_head(m1->manifests);
	list2 = list_head(m2->manifests);

	while (list1 && list2) { /* m1/file1 matches m2/file2 */
		int ret;
		bool subbed1, subbed2;
		file1 = list1->data;
		file2 = list2->data;
		subbed1 = component_subscribed(subs1, file1->filename);
		subbed2 = component_subscribed(subs2, file2->filename);

		ret = str_cmp(file1->filename, file2->filename);
		if (ret == 0) {
			file1->peer = file2;
			file2->peer = file1;
			list1 = list1->next;
			list2 = list2->next;

			/* server (new) manifests will only bring in new bundles */
			if (file2->last_change > m1->version && !file2->is_deleted) {
				if (!server && subbed1 && subbed2) {
					account_changed_bundle();
				} else if (!subbed1 && subbed2) {
					account_new_bundle();
				}
			}
			if (!server && (file2->last_change > m1->version && file2->is_deleted)) {
				if (subbed1) {
					account_deleted_bundle();
				}
			}
			continue;
		}
		if (ret < 0) { /*  m1/file1 is before m2/file2 */
			/* A bundle manifest going missing from the MoM in the
			 * latest version is no longer a breaking change. */
			if (!server && subbed1) {
				account_deleted_bundle();
			}
			list1 = list1->next;
			continue;
		} /* else ret > 0  m1/file1 is after m2/file2 */
		list2 = list2->next;
		if (subbed2) {
			account_new_bundle();
		}
	}

	// Capture deleted bundles if they are at the tail end of the list
	while (!server && list1) {
		list1 = list1->next;
		account_deleted_bundle();
	}

	// Capture new bundles if they are at the tail end of the list
	while (server && list2) {
		file2 = list2->data;
		list2 = list2->next;
		bool subbed2 = component_subscribed(subs2, file2->filename);

		if (subbed2) {
			account_new_bundle();
		}
	}
}

/* if component is specified explicitly, pull in submanifest only for that
 * if component is not specified, pull in any tracked component submanifest */
struct list *recurse_manifest(struct manifest *manifest, struct list *subs, const char *component, bool server, int *err)
{
	struct list *bundles = NULL;
	struct list *list;
	struct file *file;
	struct manifest *sub;

	manifest->contentsize = 0;
	list = manifest->manifests;
	while (list) {
		file = list->data;
		list = list->next;

		if (!server && !component && !component_subscribed(subs, file->filename)) {
			continue;
		}

		if (component && !(str_cmp(component, file->filename) == 0)) {
			continue;
		}

		sub = load_manifest(file->last_change, file, manifest, false, err);
		if (!sub) {
			list_free_list_and_data(bundles, manifest_free_data);
			return NULL;
		}
		if (sub != NULL) {
			bundles = list_prepend_data(bundles, sub);
		}
	}

	return bundles;
}

/* Takes the combination of files from several manifests and remove duplicated
 * entries. Such cases happen when two bundles have the same file in their manifest with
 * different status or version (last_changed). */
struct list *consolidate_files(struct list *files)
{
	struct list *list, *next, *tmp;
	struct file *file1, *file2;

	files = list_sort(files, cmp_file_filename_is_deleted);

	/* Two pointers ("list" and "next") traverse the consolidated, filename sorted
	 * struct list of files.  The "list" pointer is marched forward through the
	 * struct list as long as it and the next do not point
	 * to two objects with the same filename.  If the name is the same, then
	 * "list" and "next" point to the first and second in a series of perhaps
	 * many objects referring to the same filename.  As we determine which file out
	 * of multiples to keep in our consolidated, deduplicated, filename sorted list
	 * there are Manifest invariants to maintain.
	 * Note that "file" may be a file, directory or symlink.
	 */
	list = list_head(files);
	while (list) {
		next = list->next;
		if (next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = next->data;

		/* If the filenames are different, nothing to consolidate, just move on. */
		if (str_cmp(file1->filename, file2->filename)) {
			list = next;
			continue;
		}

		/* From here on, file1 and file2 have a filename match. */

		/* If both files are present, this is the most common case. */
		if (!file1->is_deleted && !file2->is_deleted) {

			/* If the hashes don't match, we have an inconsistency in the
			 * manifest. Exclude both from the list. */
			if (!hash_equal(file1->hash, file2->hash)) {
				tmp = next->next;
				list_free_item(list, NULL);
				list_free_item(next, NULL);
				list = tmp;
				telemetry(TELEMETRY_CRIT,
					  "inconsistent-file-hash",
					  "filename=%s\n"
					  "hash1=%s\n"
					  "version1=%d\n"
					  "hash2=%s\n"
					  "version2=%d\n",
					  file1->filename,
					  file1->hash,
					  file1->last_change,
					  file2->hash,
					  file2->last_change);
				continue;
			}

			/* Prefer the tracked one, since that will prevent download a file
			 * that is already in the system. Then prefer the older version
			 * since it is more likely it will contain a fullfile for it in case
			 * we need to download. */
			if (file1->is_tracked && !file2->is_tracked) {
				list_free_item(next, NULL);
			} else if (!file1->is_tracked && file2->is_tracked) {
				list_free_item(list, NULL);
				list = next;
			} else if (file1->last_change <= file2->last_change) {
				list_free_item(next, NULL);
			} else {
				list_free_item(list, NULL);
				list = next;
			}
			continue;
		}

		/* If both files are deleted, pick the newer deletion. This will make an
		 * update not ignore a new deletion. */
		if (file1->is_deleted && file2->is_deleted) {
			if (file1->last_change > file2->last_change) {
				list_free_item(next, NULL);
			} else {
				list_free_item(list, NULL);
				list = next;
			}
			continue;
		}

		/* Otherwise, pick the present file. */
		if (file2->is_deleted) {
			list_free_item(next, NULL);
		} else {
			list_free_item(list, NULL);
			list = next;
		}
	}

	return list_head(list);
}

/*
 * Takes two lists of file structs and find what elements are common in both lists
 *
 * Returns a new list that contains only the files that were common between the lists
 */
static struct list *list_common_files(struct list *list1, struct list *list2)
{
	struct list *l1 = NULL;
	struct list *l2 = NULL;
	struct list *combined = NULL;
	struct list *common = NULL;
	struct list *list;
	struct file *file1;
	struct file *file2;

	l1 = list_clone(list1);
	l2 = list_clone(list2);

	combined = list_concat(l1, l2);
	combined = list_sort(combined, cmp_file_filename_is_deleted);

	for (list = list_head(combined); list; list = list->next) {
		if (list->next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = list->next->data;
		if (str_cmp(file1->filename, file2->filename) == 0) {
			common = list_prepend_data(common, file1);
		}
	}

	list_free_list(combined);

	return common;
}

/*
 * Takes a list of files, and removes files that are marked as deleted or
 * ghosted.
 *
 * Returns the filtered list.
 */
struct list *filter_out_deleted_files(struct list *files)
{
	struct list *list, *next, *ret;
	struct file *file;

	ret = files;

	list = list_head(files);
	while (list) {
		next = list->next;

		file = list->data;

		if (file->is_deleted) {
			ret = list_free_item(list, NULL);
		}

		list = next;
	}

	return list_head(ret);
}

/*
 * Takes a list of files, and removes files from the list that exist on the filesystem.
 * This is used for cases like bundle-add where we don't want to re-add files that
 * already are on the system.
 */
struct list *filter_out_existing_files(struct list *to_install_files, struct list *installed_files)
{
	struct list *common = NULL;
	struct list *list;
	struct file *file1;
	struct file *file2;

	/* get common files between both lists */
	common = list_common_files(to_install_files, installed_files);

	/* remove the common files from the to_install_files */
	to_install_files = list_concat(to_install_files, common);
	to_install_files = list_sort(to_install_files, cmp_file_filename_is_deleted);
	list = list_head(to_install_files);
	while (list) {
		if (list->next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = list->next->data;
		if (str_cmp(file1->filename, file2->filename) == 0) { // maybe check also if the hash is the same????
			/* this file is already in the system, remove both
			 * from the list */
			list = list_free_item(list->next, NULL); // returns a pointer to the previous item
			list = list_free_item(list, NULL);
			continue;
		}

		list = list->next;
	}

	return list_head(list);
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
	file->stat.st_size = long_to_ulong(stat.st_size);

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

	return;
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
	iter1 = preserver = bmanifest->files;
	iter2 = list_head(m2->files);

	while (iter1 && iter2) {
		file1 = iter1->data;
		file2 = iter2->data;
		cur_file = iter1;

		ret = str_cmp(file1->filename, file2->filename);
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
	bmanifest->files = list_head(preserver);
}

struct file *mom_search_bundle(struct manifest *mom, const char *bundlename)
{
	return list_search(mom->manifests, bundlename, cmp_file_filename_string);
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

		if (str_cmp(file->filename, filename) == 0) {
			return file;
		}
	}
	return NULL;
}

/* Ideally have the manifest sorted already by this point */
struct file **manifest_files_to_array(struct manifest *manifest)
{
	struct file **array;
	struct list *iter;
	struct file *file;
	int i = 0;

	if (!manifest) {
		return NULL;
	}

	array = malloc_or_die(sizeof(struct file *) * manifest->filecount);
	iter = list_head(manifest->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		array[i] = file;
		i++;
	}
	return array;
}

void manifest_free_array(struct file **array)
{
	FREE(array);
}

unsigned long get_manifest_list_contentsize(struct list *manifests)
{
	unsigned long total_size = 0;

	struct list *ptr = NULL;
	for (ptr = list_head(manifests); ptr; ptr = ptr->next) {
		struct manifest *subman;
		subman = ptr->data;

		total_size += subman->contentsize;
	}

	return total_size;
}

static struct manifest *load_manifest_file(struct manifest *mom, struct file *file, int *err)
{
	if (!file) {
		if (err) {
			*err = -EINVAL;
		}
		return NULL;
	}
	return load_manifest(file->last_change, file, mom, false, err);
}

int mom_get_manifests_list(struct manifest *mom, struct list **manifest_list, filter_fn_t filter_fn)
{
	struct manifest *manifest = NULL;
	struct list *list;
	int complete = 0;
	int total;
	int err = 0, last_error = 0;

	total = list_len(mom->manifests);
	for (list = mom->manifests; list; list = list->next) {
		struct file *file = list->data;

		if (filter_fn && !filter_fn(file->filename)) {
			continue;
		}

		manifest = load_manifest_file(mom, file, &err);
		complete++;
		if (!manifest) {
			info("\n"); // Progress bar
			error("Cannot load %s manifest for version %i\n",
			      file->filename, file->last_change);
			last_error = err;
		} else {
			if (manifest_list) {
				*manifest_list = list_prepend_data(*manifest_list, manifest);
			} else {
				manifest_free(manifest);
			}
		}
		progress_report(complete, total);
	}
	progress_report(total, total);

	return last_error;
}

int recurse_dependencies(struct manifest *mom, const char *bundle, struct list **manifests, filter_fn_t filter_fn)
{
	int err = 0;
	struct manifest *manifest;
	struct list *iter;

	if (filter_fn && !filter_fn(bundle)) {
		return 0;
	}

	if (list_search(*manifests, bundle, cmp_manifest_component_string)) {
		return 0;
	}

	manifest = load_manifest_file(mom, mom_search_bundle(mom, bundle), &err);
	if (!manifest) {
		return err;
	}

	*manifests = list_prepend_data(*manifests, manifest);

	for (iter = manifest->includes; iter; iter = iter->next) {
		const char *b = iter->data;
		err = recurse_dependencies(mom, b, manifests, filter_fn);
		if (err) {
			return err;
		}
	}

	if (globals.skip_optional_bundles) {
		return 0;
	}

	for (iter = manifest->optional; iter; iter = iter->next) {
		const char *b = iter->data;
		err = recurse_dependencies(mom, b, manifests, filter_fn);
		if (err) {
			return err;
		}
	}

	return 0;
}
