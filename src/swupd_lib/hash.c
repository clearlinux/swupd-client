/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2025 Intel Corporation.
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

#include <errno.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"
#include "swupd_build_variant.h"
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
	size_t digest_str_len;
	unsigned int i;

	if (data == NULL) {
		hash_set_zeros(hash);
		return;
	}

	if (HMAC(EVP_sha256(), (const void *)key, ulong_to_int(key_len), data, data_len, digest, &digest_len) == NULL) {
		hash_set_zeros(hash);
		return;
	}

	digest_str_len = MAX((digest_len * 2) + 1, SWUPD_HASH_LEN);
	digest_str = malloc_or_die(digest_str_len * sizeof(char));

	for (i = 0; i < digest_len; i++) {
		snprintf(&digest_str[i * 2], digest_str_len, "%02x", (unsigned int)digest[i]);
	}

	hash_assign(digest_str, hash);
	FREE(digest_str);
}

static void hmac_sha256_for_string(char *hash,
				   const unsigned char *key, size_t key_len,
				   const char *str)
{
	if (str == NULL) {
		hash_set_zeros(hash);
		return;
	}

	hmac_sha256_for_data(hash, key, key_len, (const unsigned char *)str, str_len(str));
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
		FREE(xattrs_blob);
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
enum swupd_code compute_hash(struct file *file, char *filename)
{
	ssize_t ret;
	char key[SWUPD_HASH_LEN];
	size_t key_len;
	unsigned char *blob;
	FILE *fl;

	if (file->is_deleted) {
		hash_set_zeros(file->hash);
		return SWUPD_OK;
	}

	hash_set_zeros(key);

	if (file->is_link) {
		char link[PATH_MAX];
		memset(link, 0, PATH_MAX);

		ret = readlink(filename, link, PATH_MAX - 1);

		if (ret >= 0) {
			link[ret] = '\0';
			hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
			hmac_sha256_for_string(file->hash,
					       (const unsigned char *)key,
					       key_len,
					       link);
			return SWUPD_OK;
		} else {
			return SWUPD_COMPUTE_HASH_ERROR;
		}
	}

	if (file->is_dir) {
		hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
		hmac_sha256_for_string(file->hash,
				       (const unsigned char *)key,
				       key_len,
				       SWUPD_HASH_DIRNAME); // Make independent of dirname
		return SWUPD_OK;
	}

	/* if we get here, this is a regular file */
	fl = fopen(filename, "r");
	if (!fl) {
		return SWUPD_COMPUTE_HASH_ERROR;
	}
	blob = mmap(NULL, file->stat.st_size, PROT_READ, MAP_PRIVATE, fileno(fl), 0);
	if (blob == MAP_FAILED && file->stat.st_size != 0) {
		fclose(fl);
		return SWUPD_COMPUTE_HASH_ERROR;
	}

	hmac_compute_key(filename, &file->stat, key, &key_len, file->use_xattrs);
	hmac_sha256_for_data(file->hash,
			     (const unsigned char *)key,
			     key_len,
			     blob,
			     file->stat.st_size);
	munmap(blob, file->stat.st_size);
	fclose(fl);
	return SWUPD_OK;
}

bool verify_file(struct file *file, char *filename)
{
	struct file local = { 0 };

	local.filename = file->filename;
	/*
	 * xattrs are currently not supported for manifest files.
	 * They are data files produced by the swupd-server and
	 * therefore do not have any of the xattrs normally
	 * set for the actual system files (like security.ima
	 * when using IMA or security.SMACK64 when using Smack).
	 */
	local.use_xattrs = !file->is_manifest;

	populate_file_struct(&local, filename);
	if (compute_hash(&local, filename) != 0) {
		return false;
	}

	/* Check if manifest hash matches local file hash */
	return hash_equal(file->hash, local.hash);
}

bool verify_file_lazy(char *filename)
{
	struct file local = { 0 };

	if (compute_hash_lazy(&local, filename) != 0) {
		return false;
	}

	return !hash_is_zeros(local.hash);
}

/* Compares the hash for BUNDLE with that listed in the Manifest.MoM.  If the
 * hash check fails, we should assume the bundle manifest is incorrect and
 * discard it. A retry should then force redownloading of the bundle manifest.
 */
int verify_bundle_hash(struct manifest *mom, struct file *bundle)
{
	struct file *current;
	char *local = NULL, *cached;
	int ret = 0;

	current = list_search(list_head(mom->manifests), bundle, cmp_file_filename);
	if (!current) {
		return -1;
	}

	local = statedir_get_manifest(current->last_change, current->filename);
	cached = statedir_get_manifest_with_hash(current->last_change, current->filename, bundle->hash);

	/* *NOTE* If the file is changed after being hardlinked, the hash will be different,
	 * but the inode will not change making swupd skip hash check thinking they still match */
	if (sys_file_is_hardlink(local, cached)) {
		goto out;
	}

	unlink(cached);
	if (!verify_file(bundle, local)) {
		warn("hash check failed for Manifest.%s for version %i. Deleting it\n", current->filename, mom->version);
		unlink(local);
		ret = -1;
	} else {
		if (link(local, cached) < 0) {
			debug("Failed to link cache %s manifest\n", local);
		}
	}

out:
	FREE(cached);
	FREE(local);
	return ret;
}
