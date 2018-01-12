/*
 *   Software Updater - client side
 *
 *      Copyright © 2017 Intel Corporation.
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
 *         Matthew Johnson <mathew.johnson@intel.com>
 *
 */

#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>

#include "swupd.h"

/* _archive_check_err(ar, ret)
 *
 * Check and print archive errors */
static int _archive_check_err(struct archive *ar, int ret)
{
	int is_fatal = 0;

	if (ret < ARCHIVE_WARN || ret == ARCHIVE_RETRY) {
		/* error was worse than a warning or error was ARCHIVE_RETRY (greater
		 * than ARCHIVE_WARN), which indicates failure with retry possible.
		 * Regardless, indicate that the operation did not succeed.  */
		fprintf(stderr, "Error: %s\n", archive_error_string(ar));
		is_fatal = 1;
	} else if (ret < ARCHIVE_OK) {
		/* operation succeeded, warning encountered */
		fprintf(stderr, "Warning: %s\n", archive_error_string(ar));
		is_fatal = 0;
	}

	return is_fatal;
}

/* copy_data(ar, aw)
 *
 * Copy archive data from ar to aw */
static int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buffer;
	size_t size;
	off_t offset;

	for (;;) {
		r = archive_read_data_block(ar, &buffer, &size, &offset);
		if (r == ARCHIVE_EOF) {
			return ARCHIVE_OK;
		} else if (r < ARCHIVE_OK) {
			return r;
		}

		r = archive_write_data_block(aw, buffer, size, offset);
		if (r < ARCHIVE_OK) {
			return r;
		}
	}
}

/* extract_to(tarfile, outputdir)
 *
 * Extracts tar archive tarfile to outputdir. Refuses to extract any object
 * whose final location would be altered by a symlink on disk. */
int extract_to(const char *tarfile, const char *outputdir)
{
	struct archive *a, *ext;
	struct archive_entry *entry;
	int flags;
	int r = 0;

	/* set which attributes we want to restore */
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_OWNER;
	flags |= ARCHIVE_EXTRACT_XATTR;

	/* set security flags */
	flags |= ARCHIVE_EXTRACT_SECURE_SYMLINKS;
	flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

	/* set up read */
	a = archive_read_new();
	r = archive_read_support_format_tar(a);
	if (_archive_check_err(a, r)) {
		goto out_read;
	}

	r = archive_read_support_filter_all(a);
	if (_archive_check_err(a, r)) {
		goto out_read;
	}

	/* set up write */
	ext = archive_write_disk_new();
	r = archive_write_disk_set_options(ext, flags);
	if (_archive_check_err(ext, r)) {
		goto out;
	}

	r = archive_write_disk_set_standard_lookup(ext);
	if (_archive_check_err(ext, r)) {
		goto out;
	}

	r = archive_read_open_filename(a, tarfile, 10240);
	if (_archive_check_err(a, r)) {
		/* could not open archive for read */
		goto out;
	}

	/* read and write loop */
	for (;;) {
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) {
			/* reached end of file */
			r = 0;
			break;
		}

		if (_archive_check_err(a, r)) {
			goto out;
		}

		/* set output directory */
		char *fullpath;
		string_or_die(&fullpath, "%s/%s", outputdir, archive_entry_pathname(entry));
		archive_entry_set_pathname(entry, fullpath);
		free(fullpath);

		/* write archive header, if successful continue to copy data */
		r = archive_write_header(ext, entry);
		if (_archive_check_err(ext, r)) {
			goto out;
		}

		if (archive_entry_size(entry) > 0) {
			r = copy_data(a, ext);
			if (_archive_check_err(ext, r)) {
				goto out;
			}
		}

		/* flush pending file attribute changes */
		r = archive_write_finish_entry(ext);
		if (_archive_check_err(ext, r)) {
			goto out;
		}
	}

	/* at this point everything else was successful, we need to set the
	 * returncode to the result of archive_write_close in case it fails */
	r = archive_write_close(ext);
	/* call _archive_check_err() for correct error message */
	_archive_check_err(ext, r);
out:
	/* archive_write_free calls archive_write_close but does not report the
	 * error. If 'goto out' is called, we already have an error we are handling
	 * so don't overwrite that returncode. */
	archive_write_free(ext);
out_read:
	archive_read_close(a);
	archive_read_free(a);
	return r;
}
