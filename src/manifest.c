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
#include "swupd-build-variant.h"
#include "swupd.h"
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

static struct manifest *alloc_manifest(int version, char *component)
{
	struct manifest *manifest;

	manifest = calloc(1, sizeof(struct manifest));
	ON_NULL_ABORT(manifest);

	manifest->version = version;
	manifest->component = strdup_or_die(component);

	return manifest;
}

static struct manifest *manifest_from_file(int version, char *component, bool header_only, bool latest, bool is_mix)
{
	FILE *infile;
	char line[MANIFEST_LINE_MAXLEN], *c, *c2;
	int count = 0;
	int deleted = 0;
	struct manifest *manifest;
	struct list *includes = NULL;
	char *filename;
	char *basedir;
	uint64_t filecount = 0;
	uint64_t contentsize = 0;
	int manifest_hdr_version;
	int manifest_enc_version;

	if (!is_mix) {
		basedir = state_dir;
	} else {
		basedir = MIX_STATE_DIR;
	}
	string_or_die(&filename, "%s/%i/Manifest.%s", basedir, version, component);

	infile = fopen(filename, "rbm");
	if (infile == NULL) {
		free_string(&filename);
		return NULL;
	}
	free_string(&filename);

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
		if (strncmp(line, "filecount:", 10) == 0) {
			filecount = strtoull(c, NULL, 10);
			if (filecount > 4000000) {
				/* Note on the number 4,000,000. We want this
				 * to be big enough to allow Manifest.Full to
				 * pass (currently about 450,000 March 18) but
				 * small enough that when multiplied by
				 * sizeof(struct file) it fits into
				 * size_t. For a system with size_t being a 32
				 * bit value, this constrains it to be less
				 * than about 6,000,000, but close to infinity
				 * for systems with 64 bit size_t.
				 */
				fprintf(stderr, "Error: preposterous (%" PRIu64 ") number of files in %s Manifest, more than 4 million skipping\n",
					filecount, component);
				goto err_close;
			}
		}
		if (strncmp(line, "contentsize:", 12) == 0) {
			contentsize = strtoull(c, NULL, 10);
			if (contentsize > 2000000000000UL) {
				fprintf(stderr, "Error: preposterous (%" PRIu64 ") size of files in %s Manifest, more than 2TB skipping\n",
					contentsize, component);
				goto err_close;
			}
		}
		if (latest && strncmp(component, "MoM", 3) == 0) {
			if (strncmp(line, "actions:", 8) == 0) {
				post_update_actions = list_prepend_data(post_update_actions, strdup_or_die(c));
				if (!post_update_actions->data) {
					fprintf(stderr, "WARNING: Unable to read post update action from Manifest.MoM. \
							Another update or verify may be required.\n");
				}
			}
		}
		if (strncmp(line, "includes:", 9) == 0) {
			includes = list_prepend_data(includes, strdup_or_die(c));
		}
	}

	manifest = alloc_manifest(version, component);
	manifest->filecount = filecount;
	manifest->contentsize = contentsize;
	manifest->manifest_version = manifest_enc_version;
	manifest->includes = includes;

	if (header_only) {
		fclose(infile);
		return manifest;
	}

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
		ON_NULL_ABORT(file);
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
		} else if (c[0] == 'I') {
			file->is_manifest = 1;
			file->is_iterative = 1;
		} else if (c[0] != '.') { /* unknown file type */
			free(file);
			goto err;
		}

		if (c[1] == 'd') {
			file->is_deleted = 1;
			deleted++;
		} else if (c[1] == 'g') {
			file->is_deleted = 1;
			file->is_ghosted = 1;
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
			/* rename flag is ignored */
		} else if (c[3] == 'm') {
			file->is_mix = 1;
			manifest->is_mix = 1;
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

		file->filename = strdup_or_die(c);
		/* If the file is an iterative manifest, copy the from_version and obtain the
		 * bundle name based on the file name */
		if (file->is_iterative == 1) {
			char *ptr;
			ptr = strrchr(file->filename, '.');
			if (ptr != NULL) {
				/* Keep only the bundle name from the file name, the filename should be
				 * something similar to bundle-name.I.20 */
				file->bundlename = strndup_or_die(file->filename, ptr - file->filename - 2);
				file->from_version = atoi(ptr + 1);
			}
		} else {
			file->bundlename = strdup_or_die(c);
		}

		/* Mark every file in a mix manifest as also being mix content since we do not
		 * have another flag to check for like we do in the MoM */
		if (is_mix && strcmp(component, "MoM") != 0) {
			file->is_mix = 1;
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
	free_string(&manifest->component);
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
		return -1;
	}

	string_or_die(&original, "%s/%i/Manifest.%s", state_dir, current, component);

	string_or_die(&deltafile, "%s/Manifest-%s-delta-from-%i-to-%i", state_dir, component, current, new);
	/* If we cannot get a delta, quit and don't mess with the file struct.
	 * Populating it early and then exiting early corrupts the manifest list */
	memset(&buf, 0, sizeof(struct stat));
	ret = stat(deltafile, &buf);
	if (ret || buf.st_size == 0) {
		string_or_die(&url, "%s/%i/Manifest-%s-delta-from-%i", content_url, new, component, current);

		ret = swupd_curl_get_file(url, deltafile);
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
		goto out;
	} else if ((ret = xattrs_compare(original, newfile)) != 0) {
		unlink(newfile);
		goto out;
	}

	/* This is OK to do because populate_file will completely rewrite the file
	 * struct contents and not leave stale data behind. Should the
	 * implementation change, this must be updated to account for uncleared or
	 * un-updated struct members. */
	populate_file_struct(file, newfile);
	ret = compute_hash(file, newfile); // MUST save new hash!
	if (ret != 0) {
		ret = -1;
	}
out:
	unlink(deltafile);
	free_string(&original);
	free_string(&url);
	free_string(&newfile);
	free_string(&deltafile);
	return ret;
}

/* TODO: This should deal with nested manifests better */
static int retrieve_manifests(int current, int version, char *component, struct file *file, bool is_mix)
{
	char *url = NULL;
	char *filename;
	char *dir = NULL;
	char *basedir;
	int ret = 0;
	struct stat sb;

	if (!is_mix) {
		basedir = state_dir;
	} else {
		basedir = MIX_STATE_DIR;
	}

	/* Check for fullfile only, we will not be keeping the .tar around */
	string_or_die(&filename, "%s/%i/Manifest.%s", state_dir, version, component);
	if (stat(filename, &sb) == 0) {
		ret = 0;
		goto out;
	}
	free_string(&filename);

	string_or_die(&dir, "%s/%i", state_dir, version);
	ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if ((ret != 0) && (errno != EEXIST)) {
		goto out;
	}

	/* FILE is not set for a MoM, only for bundle manifests */
	if (file && current < version && strcmp(component, "full") != 0) {
		if (try_delta_manifest_download(current, version, component, file) == 0) {
			return 0;
		}
	}

	/* If it's mix content just hardlink instead of curl download */
	if (is_mix) {
		string_or_die(&filename, "%s/%i/Manifest.%s.tar", state_dir, version, component);
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
	string_or_die(&filename, "%s/%i/Manifest.%s.tar", state_dir, version, component);
	string_or_die(&url, "%s/%i/Manifest.%s.tar", content_url, version, component);

	ret = swupd_curl_get_file(url, filename);
	if (ret) {
		unlink(filename);
		goto out;
	}

untar:
	ret = extract_to(filename, dir);
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
	free_string(&url);
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

/* Removes the extracted Manifest.<bundle> and accompanying tar file, cache file, and
 * the signature file */
static void remove_manifest_files(char *filename, int version, char *hash)
{
	char *file;

	fprintf(stderr, "Warning: Removing corrupt Manifest.%s artifacts and re-downloading...\n", filename);
	string_or_die(&file, "%s/%i/Manifest.%s", state_dir, version, filename);
	unlink(file);
	free_string(&file);
	string_or_die(&file, "%s/%i/Manifest.%s.tar", state_dir, version, filename);
	unlink(file);
	free_string(&file);
	string_or_die(&file, "%s/%i/Manifest.%s.sig", state_dir, version, filename);
	unlink(file);
	free_string(&file);
	if (hash != NULL) {
		string_or_die(&file, "%s/%i/Manifest.%s.%s", state_dir, version, filename, hash);
		unlink(file);
		free_string(&file);
	}
}

struct manifest *load_mom(int version, bool latest, bool mix_exists)
{
	return load_mom_err(version, latest, mix_exists, NULL);
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
struct manifest *load_mom_err(int version, bool latest, bool mix_exists, int *err)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	char *filename;
	char *url;
	char *log_cmd = NULL;
	bool retried = false;
	bool perform_sig_verify = !(migrate && mix_exists);
	bool invalid_sig = false;

verify_mom:
	ret = retrieve_manifests(version, version, "MoM", NULL, mix_exists);
	if (ret != 0) {
		fprintf(stderr, "Failed to retrieve %d MoM manifest\n", version);
		if (err) {
			*err = ret;
		}
		return NULL;
	}

	manifest = manifest_from_file(version, "MoM", false, latest, mix_exists);

	if (manifest == NULL) {
		if (retried == false) {
			remove_manifest_files("MoM", version, NULL);
			retried = true;
			goto verify_mom;
		}
		fprintf(stderr, "Failed to load %d MoM manifest\n", version);
		if (err) {
			*err = EMANIFEST_LOAD;
		}
		return NULL;
	}

	string_or_die(&filename, "%s/%i/Manifest.MoM", state_dir, version);
	string_or_die(&url, "%s/%i/Manifest.MoM", content_url, version);

	if (perform_sig_verify) {
		invalid_sig = !download_and_verify_signature(url, filename, version, mix_exists);
	}

	/* Only when migrating , ignore the locally made signature check which is guaranteed to have been signed
	 * by the user and does not come from any network source */
	if (invalid_sig) {
		if (sigcheck) {
			/* cleanup and try one more time, statedir could have got corrupt/stale */
			if (retried == false && !mix_exists) {
				free_string(&filename);
				free_string(&url);
				free_manifest(manifest);
				remove_manifest_files("MoM", version, NULL);
				retried = true;
				goto verify_mom;
			}
			fprintf(stderr, "WARNING!!! FAILED TO VERIFY SIGNATURE OF Manifest.MoM version %d\n", version);
			free_string(&filename);
			free_string(&url);
			free_manifest(manifest);
			if (err) {
				*err = ESIGNATURE;
			}
			return NULL;
		}
		fprintf(stderr, "FAILED TO VERIFY SIGNATURE OF Manifest.MoM. Operation proceeding due to\n"
				"  --nosigcheck, but system security may be compromised\n");
		string_or_die(&log_cmd, "echo \"swupd security notice:"
					" --nosigcheck used to bypass MoM signature verification failure\" | systemd-cat --priority=\"err\" --identifier=\"swupd\"");
		if (system(log_cmd)) {
			/* useless noise to suppress gcc & glibc conspiring
			 * to make us check the result of system */
		}
		free_string(&log_cmd);
	}
	/* Make a copy of the Manifest for the completion code */
	if (latest) {
		char *momcopy;
		rm_bundle_file(".MoM");
		string_or_die(&momcopy, "/bin/cp \"%s\" \"%s/%s/.MoM\" 2>/dev/null",
			      filename, path_prefix, BUNDLES_DIR);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
		(void)system(momcopy); /* If it fails it fails */
#pragma GCC diagnostic pop
		free(momcopy);
	}
	free_string(&filename);
	free_string(&url);
	return manifest;
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
struct manifest *load_manifest(int current, int version, struct file *file, struct manifest *mom, bool header_only)
{
	struct manifest *manifest = NULL;
	int ret = 0;
	bool retried = false;

retry_load:
	ret = retrieve_manifests(current, version, file->filename, file, file->is_mix);
	if (ret != 0) {
		if (file->is_iterative == 1 && file->full_manifest && !retried) {
			/* Since the iterative manifest failed to be loaded,
			 * fall back to using the full manifest */
			file->full_manifest->peer = file->peer;
			file = file->full_manifest;
			file->peer->peer = file;
			retried = true;
			goto retry_load;
		}
		fprintf(stderr, "Failed to retrieve %d %s manifest\n", version, file->filename);
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
		return NULL;
	}

	manifest = manifest_from_file(version, file->filename, header_only, false, file->is_mix);

	if (manifest == NULL) {
		if (retried == false) {
			remove_manifest_files(file->filename, version, file->hash);
			retried = true;
			goto retry_load;
		}
		fprintf(stderr, "Failed to load %d %s manifest\n", version, file->filename);
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

	ret = retrieve_manifests(version, version, "full", NULL, mix);
	if (ret != 0) {
		fprintf(stderr, "Failed to retrieve %d Manifest.full\n", version);
		return NULL;
	}

	manifest = manifest_from_file(version, "full", false, false, false);

	if (manifest == NULL) {
		fprintf(stderr, "Failed to load %d Manifest.full\n", version);
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

	char *fullname = mk_full_filename(path_prefix, file->filename);

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

	update_count = 0;
	update_skip = 0;
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
		subbed1 = component_subscribed(subs1, file1->bundlename);
		subbed2 = component_subscribed(subs2, file2->bundlename);

		ret = strcmp(file1->bundlename, file2->bundlename);
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

/* Links each iterative manifest from a list with its related full manifest from another list */
void link_iterative_manifests(struct list *iterative_manifests, struct list *full_manifests)
{
	struct list *list;
	struct file *manifest_file;

	list = iterative_manifests;
	while (list) {
		manifest_file = list->data;
		list = list->next;
		if (manifest_file->is_iterative == 1) {
			manifest_file->full_manifest = search_bundle_in_manifests(full_manifests, manifest_file->bundlename);
		}
	}
}

/* if component is specified explicitly, pull in submanifest only for that
 * if component is not specified, pull in any tracked component submanifest */
struct list *recurse_manifest(struct manifest *manifest, struct list *subs, const char *component, bool server)
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

		if (!server && !component && !component_subscribed(subs, file->bundlename)) {
			continue;
		}

		if (component && !(strcmp(component, file->bundlename) == 0)) {
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

		sub = load_manifest(version1, version2, file, manifest, false);
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
 * Takes a list of files, and removes files from the list that exist on the filesystem.
 * This is used for cases like bundle-add where we don't want to re-add files that
 * already are on the system.
 */
struct list *filter_out_existing_files(struct list *files)
{
	struct list *list, *next, *ret;
	struct file *file;
	char *filename;

	ret = files;

	list = list_head(files);
	while (list) {
		next = list->next;

		file = list->data;

		filename = mk_full_filename(path_prefix, file->filename);
		if (verify_file_lazy(filename)) {
			ret = list_free_item(list, NULL);
		}
		free_string(&filename);

		list = next;
	}

	return ret;
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
	int count = 0;

	iter = list_head(m->files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		string_or_die(&fullfile, "%s/%s", path_prefix, file->filename);
		if (swupd_rm(fullfile) != 0) {
			free_string(&fullfile);
			continue;
		}
		free_string(&fullfile);
		count++;
	}
	fprintf(stderr, "Total deleted files: %i\n", count);
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
		if (strcmp(file->bundlename, bundlename) == 0) {
			return file;
		}
	}

	return NULL;
}

/* Search for a specific Manifest in a list of Manifests */
struct file *search_bundle_in_manifests(struct list *manifests, const char *bundlename)
{
	struct list *iter = NULL;
	struct file *file;

	iter = list_head(manifests);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		if (strcmp(file->bundlename, bundlename) == 0) {
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

static int is_version_data(const char *filename)
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
		return 1;
	}
	return 0;
}

int enforce_compliant_manifest(struct file **a, struct file **b, int searchsize, int size)
{
	struct file **found;
	int ret = 0;

	qsort(b, size, sizeof(struct file *), cmpnames);
	printf("Checking manifest uniqueness...\n");
	for (int i = 0; i < searchsize; i++) {
		found = bsearch(a[i], b, size, sizeof(struct file *), bsearch_file_helper);
		if (found) {
			if (is_version_data(a[i]->filename)) {
				continue;
			}
			if (hash_equal(a[i]->hash, (*found)->hash)) {
				continue;
			}
			fprintf(stderr, "ERROR: Conflict found for file: %s\n", a[i]->filename);
			ret++;
		}
	}
	return ret; // If collisions were found, so manifest is purely additive
}

/* The function splits a list of manifests into two lists:
 * - full_manifests: this list will contain all the full manifests that were in the manifests list
 * - iterative_manifests: this list will contain all the valid iterative manifests from the manifests
 *   list, plus the full manifest of those bundles that didn't have a valid iterative manifest
 *   A valid iterative manifest is one that can be used during an update, meaning that the iterative
 *   manifest must match the "from version" provided, and that it belongs to an already installed bundle*/
void filter_iterative_manifests(struct list *manifests, int version, struct list **full_manifests, struct list **iterative_manifests)
{
	/* Split the manifests into "iterative manifests" and "full manifests" */
	struct list *list;
	struct file *file;

	for (list = manifests; list; list = list->next) {
		file = list->data;
		/* Keep only iterative manifests that are from the specified version and that are
		 * already installed */
		if (file->is_iterative == 1 && file->from_version == version && is_tracked_bundle(file->bundlename)) {
			*iterative_manifests = list_prepend_data(*iterative_manifests, file);
		} else if (file->is_iterative != 1) {
			*full_manifests = list_prepend_data(*full_manifests, file);
		}
	}

	/* Add the full manifest of those bundles that do not have a valid iterative manifest
	* into the list of iterative_manifests */
	for (list = *full_manifests; list; list = list->next) {
		file = list->data;
		if (!search_bundle_in_manifests(*iterative_manifests, file->bundlename)) {
			*iterative_manifests = list_prepend_data(*iterative_manifests, file);
		}
	}

	/* link the iterative manifest with its related full manifest */
	link_iterative_manifests(*iterative_manifests, *full_manifests);
}
