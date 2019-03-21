/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
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

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>

#include "manifest.h"
#include "swupd.h"

#define MANIFEST_LINE_MAXLEN (PATH_MAX * 2)

// strncmp helper to be used with consts
#define strlen_const(_const) sizeof(_const) - 1
#define strncmp_const(_str, _const) strncmp(_str, _const, strlen_const(_const))

#define MANIFEST_HEADER "MANIFEST\t"

struct manifest *manifest_parse(const char *component, const char *filename, bool header_only)
{
	FILE *infile;
	char line[MANIFEST_LINE_MAXLEN], *c, *c2;
	int count = 0;
	int deleted = 0;
	int err;
	struct manifest *manifest;
	struct list *includes = NULL;
	unsigned long long filecount = 0;
	unsigned long long contentsize = 0;
	int manifest_hdr_version;
	int manifest_enc_version;

	infile = fopen(filename, "rbm");
	if (infile == NULL) {
		return NULL;
	}

	/* line 1: MANIFEST\t<version> */
	line[0] = 0;
	if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
		goto err_close;
	}

	if (strncmp_const(line, MANIFEST_HEADER) != 0) {
		goto err_close;
	}

	c = line + strlen_const(MANIFEST_HEADER);
	err = strtoi_err(c, &manifest_enc_version);

	if (manifest_enc_version <= 0 || err != 0) {
		fprintf(stderr, "Error: Loaded incompatible manifest version\n");
		goto err_close;
	}

	line[0] = 0;
	while (strncmp_const(line, "\n") != 0) {
		/* read the header */
		line[0] = 0;
		if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
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

		if (strncmp_const(line, "version:") == 0) {
			err = strtoi_err(c, &manifest_hdr_version);
			if (err != 0) {
				error("Invalid manifest version on %s\n", filename);
				goto err_close;
			}
		}
		if (strncmp_const(line, "filecount:") == 0) {
			errno = 0;
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
				fprintf(stderr, "Error: preposterous (%llu) number of files in %s Manifest, more than 4 million skipping\n",
					filecount, component);
				goto err_close;
			} else if (errno != 0) {
				fprintf(stderr, "Error: Loaded incompatible manifest filecount\n");
				goto err_close;
			}
		}
		if (strncmp_const(line, "contentsize:") == 0) {
			errno = 0;
			contentsize = strtoull(c, NULL, 10);
			if (contentsize > 2000000000000UL) {
				fprintf(stderr, "Error: preposterous (%llu) size of files in %s Manifest, more than 2TB skipping\n",
					contentsize, component);
				goto err_close;
			} else if (errno != 0) {
				fprintf(stderr, "Error: Loaded incompatible manifest contentsize\n");
				goto err_close;
			}
		}

		if (strncmp_const(line, "includes:") == 0) {
			includes = list_prepend_data(includes, strdup_or_die(c));
		}
	}

	manifest = calloc(1, sizeof(struct manifest));
	ON_NULL_ABORT(manifest);

	manifest->version = manifest_hdr_version;
	manifest->component = strdup_or_die(component);
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
		if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
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
			/* ignore this file for future iterative manifest feature */
			free(file);
			continue;
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
		} else if (c[1] == 'e') {
			file->is_experimental = 1;
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

		err = strtoi_err(c, &file->last_change);
		if (file->last_change <= 0 || err != 0) {
			fprintf(stderr, "Error: Loaded incompatible manifest last change\n");
			free(file);
			goto err;
		}

		c = c2;

		file->filename = strdup_or_die(c);

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
