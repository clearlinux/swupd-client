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
 */

#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

/* Name of the tracking directory */
#define TRACKING_DIR "bundles"

/* Name of the staged directory */
#define STAGED_DIR "staged"

/* Name of the delta directory */
#define DELTA_DIR "delta"

/* Name of the download directory */
#define DOWNLOAD_DIR "download"

/* Name of the telemetry directory */
#define TELEMETRY_DIR "telemetry"

/* Name of the swupd lock file */
#define LOCK "swupd_lock"

char *statedir_get_tracking_dir(void)
{
	return sys_path_join("%s/%s", globals.state_dir, TRACKING_DIR);
}

char *statedir_get_tracking_file(const char *bundle_name)
{
	return sys_path_join("%s/%s/%s", globals.state_dir, TRACKING_DIR, bundle_name);
}

char *statedir_get_staged_dir(void)
{
	return sys_path_join("%s/%s", globals.state_dir, STAGED_DIR);
}

char *statedir_get_staged_file(char *file_hash)
{
	return sys_path_join("%s/%s/%s", globals.state_dir, STAGED_DIR, file_hash);
}

char *statedir_get_delta_dir(void)
{
	return sys_path_join("%s/%s", globals.state_dir, DELTA_DIR);
}

char *statedir_get_download_dir(void)
{
	return sys_path_join("%s/%s", globals.state_dir, DOWNLOAD_DIR);
}

char *statedir_get_fullfile_tar(char *file_hash)
{
	return sys_path_join("%s/%s/.%s.tar", globals.state_dir, DOWNLOAD_DIR, file_hash);
}

char *statedir_get_fullfile_renamed_tar(char *file_hash)
{
	return sys_path_join("%s/%s/%s.tar", globals.state_dir, DOWNLOAD_DIR, file_hash);
}

char *statedir_get_manifest_tar(int version, char *component)
{
	return sys_path_join("%s/%i/Manifest.%s.tar", globals.state_dir, version, component);
}

char *statedir_get_manifest(int version, char *component)
{
	return sys_path_join("%s/%i/Manifest.%s", globals.state_dir, version, component);
}

char *statedir_get_hashed_manifest(int version, char *component, char *manifest_hash)
{
	return sys_path_join("%s/%i/Manifest.%s.%s", globals.state_dir, version, component, manifest_hash);
}

char *statedir_get_telemetry_record(char *record)
{
	return sys_path_join("%s/%s/%s", globals.state_dir, TELEMETRY_DIR, record);
}

char *statedir_get_swupd_lock(void)
{
	return sys_path_join("%s/%s", globals.state_dir, LOCK);
}

int statedir_create_dirs(const char *path)
{
	int ret = 0;
	unsigned int i;
	char *dir;
#define STATE_DIR_COUNT (sizeof(state_dirs) / sizeof(state_dirs[0]))
	const char *state_dirs[] = { "delta", "staged", "download", "telemetry", "bundles", "3rd-party" };

	// check for existence
	if (ensure_root_owned_dir(path)) {
		// state dir doesn't exist
		if (mkdir_p(path) != 0 || chmod(path, S_IRWXU) != 0) {
			error("failed to create %s\n", path);
			return -1;
		}
	}

	for (i = 0; i < STATE_DIR_COUNT; i++) {
		string_or_die(&dir, "%s/%s", path, state_dirs[i]);
		ret = ensure_root_owned_dir(dir);
		if (ret) {
			ret = mkdir(dir, S_IRWXU);
			if (ret) {
				error("failed to create %s\n", dir);
				FREE(dir);
				return -1;
			}
		}
		FREE(dir);
	}
	/* Do a final check to make sure that the top level dir wasn't
	 * tampered with whilst we were creating the dirs */
	if (ensure_root_owned_dir(path)) {
		return -1;
	}

	/* make sure the tracking directory is not empty, if it is,
	 * mark all installed bundles as tracked */
	if (!safeguard_tracking_dir(path)) {
		debug("There was an error accessing the tracking directory %s/bundles\n", path);
	}

	return ret;
}
