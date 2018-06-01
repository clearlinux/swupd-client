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
#include <errno.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd-build-variant.h"
#include "swupd.h"
#include "xattrs.h"

void hash_assign(const char *src, char *dst)
{
	memcpy(dst, src, SWUPD_HASH_LEN - 1);
	dst[SWUPD_HASH_LEN - 1] = '\0';
}

bool hash_equal(const char *hash1, const char *hash2)
{
	if (bcmp(hash1, hash2, SWUPD_HASH_LEN - 1) == 0) {
		return true;
	} else {
		return false;
	}
}

bool hash_is_zeros(char *hash)
{
	return hash_equal("0000000000000000000000000000000000000000000000000000000000000000", hash);
}

static void hash_set_zeros(char *hash)
{
	hash_assign("0000000000000000000000000000000000000000000000000000000000000000", hash);
}

static void hash_set_ones(char *hash)
{
	hash_assign("1111111111111111111111111111111111111111111111111111111111111111", hash);
}

static void hmac_sha256_for_data(char *hash,
				 const unsigned char *key, size_t key_len,
				 const unsigned char *data, size_t data_len)
{
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digest_len = 0;
	char *digest_str;
	unsigned int i;

	if (data == NULL) {
		hash_set_zeros(hash);
		return;
	}

	if (HMAC(EVP_sha256(), (const void *)key, key_len, data, data_len, digest, &digest_len) == NULL) {
		hash_set_zeros(hash);
		return;
	}

	digest_str = calloc((digest_len * 2) + 1, sizeof(char));
	if (digest_str == NULL) {
		abort();
	}

	for (i = 0; i < digest_len; i++) {
		sprintf(&digest_str[i * 2], "%02x", (unsigned int)digest[i]);
	}

	hash_assign(digest_str, hash);
	free_string(&digest_str);
}

static void hmac_sha256_for_string(char *hash,
				   const unsigned char *key, size_t key_len,
				   const char *str)
{
	if (str == NULL) {
		hash_set_zeros(hash);
		return;
	}

	hmac_sha256_for_data(hash, key, key_len, (const unsigned char *)str, strlen(str));
}

static void hmac_compute_key(const char *filename,
			     const struct update_stat *updt_stat,
			     char *key, size_t *key_len, bool use_xattrs)
{
	char *xattrs_blob = (void *)0xdeadcafe;
	size_t xattrs_blob_len = 0;

	if (use_xattrs) {
		xattrs_get_blob(filename, &xattrs_blob, &xattrs_blob_len);
	}

	hmac_sha256_for_data(key, (const unsigned char *)updt_stat,
			     sizeof(struct update_stat),
			     (const unsigned char *)xattrs_blob,
			     xattrs_blob_len);

	if (hash_is_zeros(key)) {
		*key_len = 0;
	} else {
		*key_len = SWUPD_HASH_LEN - 1;
	}

	if (xattrs_blob_len != 0) {
		free_string(&xattrs_blob);
	}
}

/* provide a wrapper for compute_hash() because we want a cheap-out option in
 * case we are looking for missing files only:
 *   zeros hash: file missing
 *   ones hash:  file present */
int compute_hash_lazy(struct file *file, char *filename)
{
	struct stat sb;
	if (lstat(filename, &sb) == 0) {
		hash_set_ones(file->hash);
	} else {
		hash_set_zeros(file->hash);
	}
	return 0;
}

/* this function MUST be kept in sync with the server
 * return is -1 if there was an error. If the file does not exist,
 * a "0000000..." hash is returned as is our convention in the manifest
 * for deleted files.  Otherwise file->hash is set to a non-zero hash. */
/* TODO: how should we properly handle compute_hash() failures? */
int compute_hash(struct file *file, char *filename)
{
	int ret;
	char key[SWUPD_HASH_LEN];
	size_t key_len;
	unsigned char *blob;
	FILE *fl;

	if (file->is_deleted) {
		hash_set_zeros(file->hash);
		return 0;
	}

	hash_set_zeros(key);

	if (file->is_link) {
		char link[PATH_MAXLEN];
		memset(link, 0, PATH_MAXLEN);

		ret = readlink(filename, link, PATH_MAXLEN - 1);

		if (ret >= 0) {
			hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
			hmac_sha256_for_string(file->hash,
					       (const unsigned char *)key,
					       key_len,
					       link);
			return 0;
		} else {
			return -1;
		}
	}

	if (file->is_dir) {
		hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
		hmac_sha256_for_string(file->hash,
				       (const unsigned char *)key,
				       key_len,
				       SWUPD_HASH_DIRNAME); //Make independent of dirname
		return 0;
	}

	/* if we get here, this is a regular file */
	fl = fopen(filename, "r");
	if (!fl) {
		return -1;
	}
	blob = mmap(NULL, file->stat.st_size, PROT_READ, MAP_PRIVATE, fileno(fl), 0);
	if (blob == MAP_FAILED && file->stat.st_size != 0) {
		abort();
	}

	hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
	hmac_sha256_for_data(file->hash,
			     (const unsigned char *)key,
			     key_len,
			     blob,
			     file->stat.st_size);
	munmap(blob, file->stat.st_size);
	fclose(fl);
	return 0;
}

bool verify_file(struct file *file, char *filename)
{
	struct file *local = calloc(1, sizeof(struct file));

	if (local == NULL) {
		abort();
	}

	local->filename = file->filename;
	/*
	 * xattrs are currently not supported for manifest files.
	 * They are data files produced by the swupd-server and
	 * therefore do not have any of the xattrs normally
	 * set for the actual system files (like security.ima
	 * when using IMA or security.SMACK64 when using Smack).
	 */
	local->use_xattrs = !file->is_manifest;

	populate_file_struct(local, filename);
	if (compute_hash(local, filename) != 0) {
		free(local);
		return false;
	}

	/* Check if manifest hash matches local file hash */
	if (hash_equal(file->hash, local->hash)) {
		free(local);
		return true;
	} else {
		free(local);
		return false;
	}
}

/* Compares the hash for BUNDLE with that listed in the Manifest.MoM.  If the
 * hash check fails, we should assume the bundle manifest is incorrect and
 * discard it. A retry should then force redownloading of the bundle manifest.
 */
int verify_bundle_hash(struct manifest *manifest, struct file *bundle)
{
	struct list *iter = list_head(manifest->manifests);
	struct file *current;
	char *local = NULL, *cached;
	int ret = 0;

	while (iter) {
		struct stat sb, sb2;

		current = iter->data;
		iter = iter->next;

		if (strcmp(current->filename, bundle->filename) != 0) {
			continue;
		}

		string_or_die(&cached, "%s/%i/Manifest.%s.%s", state_dir,
			      current->last_change, current->filename, bundle->hash);

		string_or_die(&local, "%s/%i/Manifest.%s", state_dir,
			      current->last_change, current->filename);

		/* *NOTE* If the file is changed after being hardlinked, the hash will be different,
		 * but the inode will not change making swupd skip hash check thinking they still match */
		if (stat(local, &sb) == 0 && stat(cached, &sb2) == 0 && sb.st_ino == sb2.st_ino) {
			free_string(&cached);
			break;
		} else {
			unlink(cached);
		}

		if (stat(local, &sb) != 0) {
			/* missing bundle manifest - attempt to download it */
			char *filename;
			char *url;

			fprintf(stderr, "Warning: Downloading missing manifest for bundle %s version %d.\n",
				current->filename, current->last_change);

			string_or_die(&filename, "%s/%i/Manifest.%s", state_dir,
				      current->last_change, current->filename);
			string_or_die(&url, "%s/%i/Manifest.%s.tar", content_url,
				      current->last_change, current->filename);
			ret = swupd_curl_get_file(url, filename, NULL, NULL, false);
			free_string(&url);

			if (ret != 0) {
				fprintf(stderr, "Error: download of %s failed\n", filename);
				unlink(filename);
				free_string(&filename);
				break;
			}

			char *outputdir;
			string_or_die(&outputdir, "%s/%i", state_dir, current->last_change);
			ret = extract_to(filename, outputdir);
			free_string(&outputdir);
			free_string(&filename);

			if (ret != 0) {
				break;
			}
		}

		if (!verify_file(bundle, local)) {
			fprintf(stderr, "Warning: hash check failed for Manifest.%s for version %i. Deleting it.\n",
				current->filename, manifest->version);
			unlink(local);
			ret = 1;
		}
		if (link(local, cached) != 0) {
			unlink(cached);
		}
		free_string(&cached);
		break;
	}

	free_string(&local);
	return ret;
}
