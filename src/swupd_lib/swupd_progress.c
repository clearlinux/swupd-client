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
 */

#include "swupd_progress.h"
#include "swupd.h"

int swupd_progress_callback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t UNUSED_PARAM ultotal, int64_t UNUSED_PARAM ulnow)
{

	struct file_progress *progress = clientp;
	long increment;

	if (progress->overall_progress->total_download_size == 0 || dltotal == 0 || (long)dlnow == progress->downloaded) {
		/* nothing new has been downloaded since the last time the
		 * periodic callback was called, just return */
		return 0;
	}

	if (progress->downloaded > (long)dlnow) {
		/* if this happened,there was probably an error with the download
		 * and it had to be started over, wait till we get back to where we were */
		return 0;
	}

	/* calculate the downloaded data size since the last
	 * time the function was called */
	increment = (long)dlnow - progress->downloaded;
	progress->downloaded = (long)dlnow;

	/* pass the increment to a common sum */
	progress->overall_progress->downloaded += increment;

	/* if more data has been downloaded, report progress */
	if (increment > 0) {
		progress_report((double)progress->overall_progress->downloaded, (double)progress->overall_progress->total_download_size);
	}

	return 0;
}
