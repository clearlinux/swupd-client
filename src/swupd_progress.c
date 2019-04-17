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

#include "swupd_progress.h"
#include "swupd.h"

int swupd_progress_callback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t UNUSED_PARAM ultotal, int64_t UNUSED_PARAM ulnow)
{
	struct download_progress *download_progress;
	double increment;

	download_progress = clientp;

	if (download_progress->total_download_size == 0) {
		return 0;
	}

	/* calculate the downloaded data size since the last
	 * time the function was called */
	if (dlnow < download_progress->dlprev) {
		/* new file */
		download_progress->dlprev = 0;
	}
	increment = dlnow - download_progress->dlprev;
	download_progress->dlprev = dlnow;
	download_progress->current += increment;

	/* if more data has been downloaded, report progress */
	if (dltotal && increment > 0) {
		progress_report(download_progress->current, download_progress->total_download_size);
	}

	return 0;
}
