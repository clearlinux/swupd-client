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
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"
#include "swupd_build_variant.h"
#include "xattrs.h"

#define MANIFEST_LINE_MAXLEN 8192

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

int file_sort_hash(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;
	A = (struct file *)a;
	B = (struct file *)b;

	ret = strcmp(A->hash, B->hash);

	return ret;
}

static struct manifest *manifest_from_file(int version, char *component, bool header_only, bool is_mix)
{
	char *filename;
	char *basedir;
	struct manifest *manifest;

	if (!is_mix) {
		basedir = globals.state_dir;
	} else {
		basedir = MIX_STATE_DIR;
	}

	string_or_die(&filename, "%s/%i/Manifest.%s", basedir, version, component);
	manifest = manifest_parse(component, filename, header_only);
	free(filename);

	if (!manifest) {
		return NULL;
	}

	if (manifest->version != version) {
		error("Loaded incompatible manifest header version for %s: %d != %d\n", component, manifest->version, version);
		free_manifest(manifest);

		return NULL;
	}

	/* Mark every file in a mix manifest as also being mix content since we do not
	 * have another flag to check for like we do in the MoM */
	if (strcmp(component, "MoM") != 0) {
		if (is_mix != 0) {
			struct list *l;
			for (l = manifest->manifests; l; l = l->next) {
				struct file *file = l->data;

				file->is_mix = 1;
			}
		}
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

	if (strcmp(component, "MoM") == 0) {
		// We don't do MoM deltas.
		return -1;
	}

	if (strcmp(component, "full") == 0) {
		// We don't do full manifest deltas.
		return -1;
	}

	string_or_die(&from_manifest, "%s/%i/Manifest.%s", globals.state_dir, from, component);
	string_or_die(&to_manifest, "%s/%i/Manifest.%s", globals.state_dir, to, component);
	string_or_die(&manifest_delta, "%s/Manifest-%s-delta-from-%i-to-%i", globals.state_dir, component, from, to);
	string_or_die(&to_dir, "%s/%i", globals.state_dir, to);

	if (!file_exists(manifest_delta)) {
		string_or_die(&url, "%s/%i/Manifest-%s-delta-from-%i", globals.content_url, to, component, from);
		ret = swupd_curl_get_file(url, manifest_delta);
		free_string(&url);
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
	free_string(&to_manifest);
	free_string(&to_dir);
	free_string(&from_manifest);
	free_string(&manifest_delta);
	return ret;
}

/* TODO: This should deal with nested manifests better */
static int retrieve_manifest(int previous_version, int version, char *component, bool is_mix)
{
	char *url = NULL;
	char *filename;
	char *filename_cache = NULL;
	char *dir = NULL;
	char *basedir;
	int ret = 0;
	struct stat sb;

	if (!is_mix) {
		basedir = globals.state_dir;
	} else {
		basedir = MIX_STATE_DIR;
	}

	/* Check for fullfile only, we will not be keeping the .tar around */
	string_or_die(&filename, "%s/%i/Manifest.%s", globals.state_dir, version, component);
	if (stat(filename, &sb) == 0) {
		ret = 0;
		goto out;
	}

	string_or_die(&dir, "%s/%i", globals.state_dir, version);
	ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if ((ret != 0) && (errno != EEXIST)) {
		goto out;
	}

	/* Check statedir-cache */
	if (globals.state_dir_cache != NULL) {
		string_or_die(&filename_cache, "%s/%i/Manifest.%s", globals.state_dir_cache, version, component);
		if (link_or_copy(filename_cache, filename) == 0) {
			ret = 0;
			goto out;
		}
	}

	if (try_manifest_delta_download(previous_version, version, component) == 0) {
		ret = 0;
		goto out;
	}
	free_string(&filename);

	/* If it's mix content just hardlink instead of curl download */
	if (is_mix) {
		string_or_die(&filename, "%s/%i/Manifest.%s.tar", globals.state_dir, version, component);
		string_or_die(&url, "%s/%i/Manifest.%s.tar", basedir, version, component);
		ret = link_or_rename(url, filename);
		/* If rename fails, we try again below with curl to the contenurl */
		if (ret == 0) {
			goto untar;
		}
		free_string(&filename);
		free_string(&url);
	}

	/* Either we're not on mix or it failed, try curl-ing the file if link didn't work */
	string_or_die(&filename, "%s/%i/Manifest.%s.tar", globals.state_dir, version, component);
	string_or_die(&url, "%s/%i/Manifest.%s.tar", globals.content_url, version, component);

	ret = swupd_curl_get_file(url, filename);
	if (ret) {
		unlink(filename);
		goto out;
	}

untar:
	ret = archives_extract_to(filename, dir);
	if (ret != 0) {
		goto out;
	} else {
		if (!is_mix) {
			unlink(filename);
		}
	}

out:
	free_string(&dir);
	free_string(&filename);
	free_string(&filename_cache);
	free_string(&url);
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

/* Removes the extracted Manifest.<bundle> and accompanying tar file, cache file, and
 * the signature file */
static void remove_manifest_files(char *filename, int version, char *hash)
{
	char *file;

	warn("Removing corrupt Manifest.%s artifacts and re-downloading...\n", filename);
	string_or_die(&file, "%s/%i/Manifest.%s", globals.state_dir, version, filename);
	unlink(file);
	free_string(&file);
	string_or_die(&file, "%s/%i/Manifest.%s.tar", globals.state_dir, version, filename);
	unlink(file);
	free_string(&file);
	string_or_die(&file, "%s/%i/Manifest.%s.sig", globals.state_dir, version, filename);
	unlink(file);
	free_string(&file);
	if (hash != NULL) {
		string_or_die(&file, "%s/%i/Manifest.%s.%s", globals.state_dir, version, filename, hash);
		unlink(file);
		free_string(&file);
	}
}

/* Verifies signature for the local file DATA_FILENAME first, and on failure
 * downloads the signature based on DATA_URL and tries to verify again.
 * Automatically manages the signature for mix content, performing local
 * verification only if the manifest is user created.
 *
 * returns: true if signature verification succeeded, false if verification
 * failed, or the signature download failed
 */
bool mom_signature_verify(const char *data_url, const char *data_filename, int version, bool mix_exists)
{
	char *local = NULL;
	char *sig_url = NULL;
	char *sig_filename = NULL;
	char *sig_filename_cache = NULL;
	int ret;
	bool result;

	string_or_die(&sig_url, "%s.sig", data_url);
	string_or_die(&sig_filename, "%s.sig", data_filename);
	string_or_die(&local, "%s/%i/Manifest.MoM.sig", MIX_STATE_DIR, version);

	// Try verifying a local copy of the signature first
	result = signature_verify(data_filename, sig_filename, false);
	if (result) {
		goto out;
	}

	// Check statedir-cache
	if (globals.state_dir_cache != NULL) {
		string_or_die(&sig_filename_cache, "%s/%i/Manifest.MoM.sig", globals.state_dir_cache, version);
		if (link_or_copy(sig_filename_cache, sig_filename) == 0) {
			result = signature_verify(data_filename, sig_filename, false);
			if (result) {
				goto out;
			}
		}
	}

	// Else, download a fresh signature, and verify
	if (mix_exists) {
		ret = link(local, sig_filename);
	} else {
		ret = swupd_curl_get_file(sig_url, sig_filename);
	}

	if (ret == 0) {
		result = signature_verify(data_filename, sig_filename, true);
	} else {
		// download failed
		result = false;
	}
out:
	free_string(&local);
	free_string(&sig_filename);
	free_string(&sig_url);
	free_string(&sig_filename_cache);
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
struct manifest *load_mom(int version, bool latest, bool mix_exists, int *err)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	char *filename;
	char *url;
	bool retried = false;
	bool needs_sig_verification = !(globals.migrate && mix_exists);

retry_load:
	ret = retrieve_manifest(0, version, "MoM", mix_exists);
	if (ret != 0) {
		error("Failed to retrieve %d MoM manifest\n", version);
		if (err) {
			*err = ret;
		}
		return NULL;
	}

	manifest = manifest_from_file(version, "MoM", false, mix_exists);

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

	string_or_die(&filename, "%s/%i/Manifest.MoM", globals.state_dir, version);
	string_or_die(&url, "%s/%i/Manifest.MoM", globals.content_url, version);

	if (!globals.sigcheck) {
		warn("FAILED TO VERIFY SIGNATURE OF Manifest.MoM. Operation proceeding due to\n"
		     "  --nosigcheck, but system security may be compromised\n");
		journal_log_error("swupd security notice: --nosigcheck used to bypass MoM signature verification failure");
	} else {
		/* Only when migrating , ignore the locally made signature check which is guaranteed to have been signed
		 * by the user and does not come from any network source */
		if (needs_sig_verification && !mom_signature_verify(url, filename, version, mix_exists)) {
			/* cleanup and try one more time, statedir could have got corrupt/stale */
			if (retried == false && !mix_exists) {
				free_string(&filename);
				free_string(&url);
				free_manifest(manifest);
				remove_manifest_files("MoM", version, NULL);
				retried = true;
				goto retry_load;
			}
			error("FAILED TO VERIFY SIGNATURE OF Manifest.MoM version %d!!!\n", version);
			free_string(&filename);
			free_string(&url);
			free_manifest(manifest);
			if (err) {
				*err = SWUPD_SIGNATURE_VERIFICATION_FAILED;
			}
			return NULL;
		}
	}

	/* Make a copy of the Manifest for the completion code */
	if (latest) {
		char *momdir;
		char *momfile;

		string_or_die(&momdir, "%s/var/tmp/swupd", globals.path_prefix);
		string_or_die(&momfile, "%s/Manifest.MoM", momdir);
		swupd_rm(momfile);
		mkdir_p(momdir);

		copy(filename, momfile);
		free_string(&momdir);
		free_string(&momfile);
	}
	free_string(&filename);
	free_string(&url);
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
	ret = retrieve_manifest(prev_version, version, file->filename, file->is_mix);
	if (ret != 0) {
		error("Failed to retrieve %d %s manifest\n", version, file->filename);
		if (err) {
			*err = ret;
		}
		return NULL;
	}

	if (!header_only) {
		ret = verify_bundle_hash(mom, file);
	}

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

	manifest = manifest_from_file(version, file->filename, header_only, file->is_mix);

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

/* Special case manifest for mixer content enforcement */
struct manifest *load_manifest_full(int version, bool mix)
{
	struct manifest *manifest = NULL;
	int ret = 0;

	ret = retrieve_manifest(0, version, "full", mix);
	if (ret != 0) {
		error("Failed to retrieve %d Manifest.full\n", version);
		return NULL;
	}

	manifest = manifest_from_file(version, "full", false, false);

	if (manifest == NULL) {
		error("Failed to load %d Manifest.full\n", version);
		return NULL;
	}

	return manifest;
}

/* Checks to see if the given file is installed under the path_prefix. */
static bool is_installed_and_verified(struct file *file)
{
	/* Not safe to perform the hash check if there was a type change
	 * involving symlinks. */
	if (file->is_link != file->peer->is_link) {
		return false;
	}

	char *fullname = mk_full_filename(globals.path_prefix, file->filename);

	if (verify_file(file, fullname)) {
		free_string(&fullname);
		return true;
	}
	free_string(&fullname);
	return false;
}

/* Find files which need updated based on differences in last_change.
   Should let further do_not_update policy be handled in the caller, but for
   now some hacky exclusions are done here. */
struct list *create_update_list(struct manifest *server)
{
	struct list *output = NULL;
	struct list *list;

	globals.update_count = 0;
	globals.update_skip = 0;
	list = list_head(server->files);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;

		/* Look for potential short circuit, if something has the same
		 * flags and the same hash, then conclude they are the same. */
		if (file->peer &&
		    file->is_deleted == file->peer->is_deleted &&
		    file->is_file == file->peer->is_file &&
		    file->is_dir == file->peer->is_dir &&
		    file->is_link == file->peer->is_link &&
		    hash_equal(file->hash, file->peer->hash)) {
			if (file->last_change == file->peer->last_change) {
				/* Nothing to do; the file did not change */
				continue;
			}
			/* When file and its peer have matching hashes
			 * but different versions, this indicates a
			 * minversion bump was performed server-side.
			 * Skip updating them if installed with the
			 * correct hash. */
			if (is_installed_and_verified(file)) {
				continue;
			}
		}

		/* Note: at this stage, "untracked" files are always "new"
		 * files, so they will not have a peer. */
		if (!file->peer ||
		    (file->peer && file->last_change > file->peer->last_change)) {
			/* check and if needed mark as do_not_update */
			(void)ignore(file);
			/* check if we need to run scripts/update the bootloader/etc */
			apply_heuristics(file);

			output = list_prepend_data(output, file);
			continue;
		}
	}
	globals.update_count = list_len(output) - globals.update_skip;

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

	m1->manifests = list_sort(m1->manifests, file_sort_filename);
	m2->manifests = list_sort(m2->manifests, file_sort_filename);

	list1 = list_head(m1->manifests);
	list2 = list_head(m2->manifests);

	while (list1 && list2) { /* m1/file1 matches m2/file2 */
		int ret;
		bool subbed1, subbed2;
		file1 = list1->data;
		file2 = list2->data;
		subbed1 = component_subscribed(subs1, file1->filename);
		subbed2 = component_subscribed(subs2, file2->filename);

		ret = strcmp(file1->filename, file2->filename);
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
			 * latest version is a breaking change, only possible
			 * during a format bump, so don't account for this
			 * possibility in the stats. */
			list1 = list1->next;
			continue;
		} /* else ret > 0  m1/file1 is after m2/file2 */
		list2 = list2->next;
		if (subbed2) {
			account_new_bundle();
		}
	}

	// Capture new bundles if they are at the tail end of the list
	while (list2) {
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

		if (component && !(strcmp(component, file->filename) == 0)) {
			continue;
		}

		sub = load_manifest(file->last_change, file, manifest, false, err);
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

/* Takes the combination of files from several manifests and remove duplicated
 * entries. Such cases happen when two bundles have the same file in their manifest with
 * different status or version (last_changed). */
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
		if (strcmp(file1->filename, file2->filename)) {
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

	return list;
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
	combined = list_sort(combined, file_sort_filename);

	for (list = list_head(combined); list; list = list->next) {
		if (list->next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = list->next->data;
		if (strcmp(file1->filename, file2->filename) == 0) {
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
	to_install_files = list_sort(to_install_files, file_sort_filename);
	list = list_head(to_install_files);
	while (list) {
		if (list->next == NULL) {
			break;
		}
		file1 = list->data;
		file2 = list->next->data;
		if (strcmp(file1->filename, file2->filename) == 0) { // maybe check also if the hash is the same????
			/* this file is already in the system, remove both
			 * from the list */
			list = list_free_item(list->next, NULL); // returns a pointer to the previous item
			list = list_free_item(list, NULL);
			continue;
		}

		list = list->next;
	}

	return list;
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

	return;
}

/* Iterate m file list and remove from filesystem each file/directory */
void remove_files_in_manifest_from_fs(struct manifest *m)
{
	struct list *iter = NULL;
	struct file *file = NULL;
	char *fullfile = NULL;
	int count = list_len(m->files);

	iter = list_head(m->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		string_or_die(&fullfile, "%s/%s", globals.path_prefix, file->filename);
		if (swupd_rm(fullfile) == -1) {
			/* if a -1 is returned it means there was an issue deleting the
			 * file or directory, in that case decrease the counter of deleted
			 * files.
			 * Note: If a file didn't exist it will still be counted as deleted,
			 * this is a limitation */
			count--;
		}
		free_string(&fullfile);
	}
	info("Total deleted files: %i\n", count);
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

/* Ideally have the manifest sorted already by this point */
struct file **manifest_files_to_array(struct manifest *manifest)
{
	struct file **array;
	struct list *iter;
	struct file *file;
	int i = 0;
	int numfiles;

	if (!manifest) {
		return NULL;
	}

	numfiles = manifest->filecount;

	array = malloc(sizeof(struct file *) * numfiles);
	ON_NULL_ABORT(array);

	iter = list_head(manifest->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		array[i] = file;
		i++;
	}
	return array;
}

void free_manifest_array(struct file **array)
{
	free(array);
}

static int cmpnames(const void *a, const void *b)
{
	return strcmp((*(struct file **)a)->filename, (*(struct file **)b)->filename);
}

static bool is_version_data(const char *filename)
{
	if (strcmp(filename, "/usr/lib/os-release") == 0 ||
	    strcmp(filename, "/usr/share/clear/version") == 0 ||
	    strcmp(filename, "/usr/share/clear/versionstamp") == 0 ||
	    strcmp(filename, DEFAULT_VERSION_URL_PATH) == 0 ||
	    strcmp(filename, DEFAULT_CONTENT_URL_PATH) == 0 ||
	    strcmp(filename, DEFAULT_FORMAT_PATH) == 0 ||
	    strcmp(filename, "/usr/share/clear/update-ca/Swupd_Root.pem") == 0 ||
	    strcmp(filename, "/usr/share/clear/os-core-update-index") == 0 ||
	    strcmp(filename, "/usr/share/clear/allbundles/os-core") == 0) {
		return true;
	}
	return false;
}

int enforce_compliant_manifest(struct file **a, struct file **b, int searchsize, int size)
{
	struct file **found;
	int ret = 0;

	qsort(b, size, sizeof(struct file *), cmpnames);
	info("Checking manifest uniqueness...\n");
	for (int i = 0; i < searchsize; i++) {
		found = bsearch(a[i], b, size, sizeof(struct file *), bsearch_file_helper);
		if (found) {
			if (is_version_data(a[i]->filename)) {
				continue;
			}
			if (hash_equal(a[i]->hash, (*found)->hash)) {
				continue;
			}
			error("Conflict found for file: %s\n", a[i]->filename);
			ret++;
		}
	}
	return ret; // If collisions were found, so manifest is purely additive
}

long get_manifest_list_contentsize(struct list *manifests)
{
	long total_size = 0;

	struct list *ptr = NULL;
	for (ptr = list_head(manifests); ptr; ptr = ptr->next) {
		struct manifest *subman;
		subman = ptr->data;

		total_size += subman->contentsize;
	}

	return total_size;
}
