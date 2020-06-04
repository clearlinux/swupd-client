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

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

/* Name of the tracking directory */
#define TRACKING_DIR "bundles"

/* Name of the telemetry directory */
#define TELEMETRY_DIR "telemetry"

/* Name of the swupd lock file */
#define LOCK "swupd_lock"

/* Name of the version file */
#define VERSION_FILE "version"

/* Name of the directory that contains all the cache */
#define CACHE_DIR "cache"

/* Name of the delta directory */
#define DELTA_DIR "delta"

/* Name of the download directory */
#define DOWNLOAD_DIR "download"

/* Name of the manifest directory */
#define MANIFEST_DIR "manifest"

/* Name of the staged directory */
#define STAGED_DIR "staged"

/* Name of the temp directory */
#define TEMP_DIR "temp"

/* Name of the directory that contains cache/data of a 3rd-party repo */
#define THIRD_PARTY_DIR "3rd-party"

/* Permissions for a root only directory */
#define ROOT_ONLY S_IRWXU

/* Permissions for a directory readable by users */
#define WORLD_READABLE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)


/* ******************** */
/* Data (state) section */
/* ******************** */

/* All this data is relative to the path_prefix of the system by default */

char *statedir_get_tracking_dir(void)
{
	return sys_path_join("%s/%s", globals.data_dir, TRACKING_DIR);
}

char *statedir_get_tracking_file(const char *bundle_name)
{
	return sys_path_join("%s/%s/%s", globals.data_dir, TRACKING_DIR, bundle_name);
}

char *statedir_get_telemetry_record(char *record)
{
	return sys_path_join("%s/%s/%s", globals.data_dir, TELEMETRY_DIR, record);
}

char *statedir_get_swupd_lock(void)
{
	return sys_path_join("%s/%s", globals.data_dir, LOCK);
}

char *statedir_get_version(void)
{
	return sys_path_join("%s/%s", globals.data_dir, VERSION_FILE);
}

/* ************* */
/* Cache section */
/* ************* */

static char *convert_url(const char *url)
{
	size_t i = 0;
	char *converted_url = malloc_or_die(str_len(url) + 1);

	// replace ':' and '/' with '_'
	for (i = 0; i < str_len(url); i++) {
		if (url[i] == ':' || url[i] == '/') {
			converted_url[i] = '_';
		} else {
			converted_url[i] = url[i];
		}
	}
	converted_url[i] = '\0';

	return converted_url;
}

static char *get_url(void)
{
	return convert_url(globals.content_url);
}

char *statedir_get_cache_dir(void)
{
	return sys_path_join("%s/%s", globals.cache_dir, CACHE_DIR);
}

char *statedir_get_cache_url_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s", globals.cache_dir, CACHE_DIR, url);
	FREE(url);

	return dir;
}

char *statedir_get_delta_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, DELTA_DIR);
	FREE(url);

	return dir;
}

char *statedir_get_delta_pack_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s", globals.cache_dir, CACHE_DIR, url);
	FREE(url);

	return dir;
}

static char *get_delta_pack(char *state, char *bundle, int from_version, int to_version)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/pack-%s-from-%i-to-%i.tar", state, CACHE_DIR, url, bundle, from_version, to_version);
	FREE(url);

	return path;
}

char *statedir_get_delta_pack(char *bundle, int from_version, int to_version)
{
	return get_delta_pack(globals.cache_dir, bundle, from_version, to_version);
}

char *statedir_dup_get_delta_pack(char *bundle, int from_version, int to_version)
{
	return get_delta_pack(globals.state_dir_cache, bundle, from_version, to_version);
}

char *statedir_get_download_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, DOWNLOAD_DIR);
	FREE(url);

	return dir;
}

char *statedir_get_download_file(char * filename)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, DOWNLOAD_DIR, filename);
	FREE(url);

	return path;
}

char *statedir_get_manifest_root_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, MANIFEST_DIR);
	FREE(url);

	return dir;
}

static char *get_manifest_dir(char *state, int version)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s/%i", state, CACHE_DIR, url, MANIFEST_DIR, version);
	FREE(url);

	return dir;
}

char *statedir_get_manifest_dir(int version)
{
	return get_manifest_dir(globals.cache_dir, version);
}

char *statedir_dup_get_manifest_dir(int version)
{
	return get_manifest_dir(globals.state_dir_cache, version);
}

static char *get_manifest(char *state, int version, char *component)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%i/Manifest.%s", state, CACHE_DIR, url, MANIFEST_DIR, version, component);
	FREE(url);

	return path;
}

char *statedir_get_manifest(int version, char *component)
{
	return get_manifest(globals.cache_dir, version, component);
}

char *statedir_dup_get_manifest(int version, char *component)
{
	return get_manifest(globals.state_dir_cache, version, component);
}

char *statedir_get_manifest_tar(int version, char *component)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%i/Manifest.%s.tar", globals.cache_dir, CACHE_DIR, url, MANIFEST_DIR, version, component);
	FREE(url);

	return path;
}

char *statedir_get_manifest_with_hash(int version, char *component, char *manifest_hash)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%i/Manifest.%s.%s", globals.cache_dir, CACHE_DIR, url, MANIFEST_DIR, version, component, manifest_hash);
	FREE(url);

	return path;
}

char *statedir_get_manifest_delta_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, MANIFEST_DIR);
	FREE(url);

	return dir;
}

char *statedir_get_manifest_delta(char *bundle, int from_version, int to_version)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/Manifest-%s-delta-from-%i-to-%i", globals.cache_dir, CACHE_DIR, url, MANIFEST_DIR, bundle, from_version, to_version);
	FREE(url);

	return path;
}


char *statedir_get_staged_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, STAGED_DIR);
	FREE(url);

	return dir;
}

static char *get_staged_file(char *state, char *file_hash)
{
	char *url = get_url();
	char *path =  sys_path_join("%s/%s/%s/%s/%s", state, CACHE_DIR, url, STAGED_DIR, file_hash);
	FREE(url);

	return path;
}

char *statedir_get_staged_file(char *file_hash)
{
	return get_staged_file(globals.cache_dir, file_hash);
}

char *statedir_dup_get_staged_file(char *file_hash)
{
	return get_staged_file(globals.state_dir_cache, file_hash);
}

char *statedir_get_fullfile_tar(char *file_hash)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/.%s.tar", globals.cache_dir, CACHE_DIR, url, DOWNLOAD_DIR, file_hash);
	FREE(url);

	return path;
}

char *statedir_get_fullfile_renamed_tar(char *file_hash)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%s.tar", globals.cache_dir, CACHE_DIR, url, DOWNLOAD_DIR, file_hash);
	FREE(url);

	return path;
}

char *statedir_get_temp_dir(void)
{
	char *url = get_url();
	char *dir = sys_path_join("%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, TEMP_DIR);
	FREE(url);

	return dir;
}

char *statedir_get_temp_file(char *filename)
{
	char *url = get_url();
	char *path = sys_path_join("%s/%s/%s/%s/%s", globals.cache_dir, CACHE_DIR, url, TEMP_DIR, filename);
	FREE(url);

	return path;
}


/* *************** */
/* Setup functions */
/* *************** */

static int create_dir(const char *path, const char *dirname, mode_t mode)
{
	int ret = 0;
	char *dir = NULL;

	// if the path to the new directory doesn't exist, create it
	if (!sys_is_dir(path)) {
		if (mkdir_p(path) || chmod(path, WORLD_READABLE)) {
			return -1;
		}
	}

	// if the directory already exist and has correct permissions
	// we are done
	dir = sys_path_join("%s/%s", path, dirname);
	if (sys_is_dir(dir) && sys_is_mode(dir, mode)) {
		goto exit;
	}

	// remove the existing directory recursively (if any)
	ret = sys_rm_recursive(dir);
	if (ret != 0 && ret != -ENOENT) {
		error("%s is the wrong type or has the wrong permissions but failed to be removed\n", dir);
		goto exit;
	}

	// create the directory with the right permissions
	ret = mkdir(dir, mode);
	if (ret) {
		error("failed to create directory %s\n", dir);
	}

exit:
	FREE(dir);

	return ret;
}

static int create_cache_dirs(void)
{
	int ret = 0;
	char *sub_path = NULL;
	char *url = get_url();
	const char *cache_dirs[] = { DELTA_DIR, STAGED_DIR, DOWNLOAD_DIR, MANIFEST_DIR, TEMP_DIR };

	// create the "cache" directory
	ret = create_dir(globals.cache_dir, CACHE_DIR, WORLD_READABLE);
	if (ret) {
		goto exit;
	}

	// create the url-dependent directory
	sub_path = sys_path_join("%s/%s", globals.cache_dir, CACHE_DIR);
	ret = create_dir(sub_path, url, WORLD_READABLE);
	if (ret) {
		goto exit;
	}

	// create all cache directories
	FREE(sub_path);
	sub_path = sys_path_join("%s/%s/%s", globals.cache_dir, CACHE_DIR, url);
	for (unsigned int i = 0; i < sizeof(cache_dirs) / sizeof(cache_dirs[0]); i++) {
		if (str_cmp(cache_dirs[i], MANIFEST_DIR) == 0) {
			// access to the manifest dir should not be restricted
			ret = create_dir(sub_path, cache_dirs[i], WORLD_READABLE);
		} else {
			// access to the rest of the cache dirs should be restricted to root only
			ret = create_dir(sub_path, cache_dirs[i], ROOT_ONLY);
		}
		if (ret) {
			goto exit;
		}
	}

exit:
	FREE(url);
	FREE(sub_path);

	return ret;
}

static int create_data_dirs(bool include_all)
{
	int ret = 0;
	unsigned int count = 0;
	const char *data_dirs[] = { TRACKING_DIR, TELEMETRY_DIR, THIRD_PARTY_DIR };
	const char *data_dirs_for_repos[] = { TRACKING_DIR };
	const char **selected_set = NULL;

	if (include_all) {
		selected_set = data_dirs;
		count = sizeof(data_dirs) / sizeof(data_dirs[0]);
	} else {
		selected_set = data_dirs_for_repos;
		count = sizeof(data_dirs_for_repos) / sizeof(data_dirs_for_repos[0]);
	}

	// create all data directories
	for (unsigned int i = 0; i < count; i++) {
		ret = create_dir(globals.data_dir, selected_set[i], WORLD_READABLE);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

int statedir_create_dirs(bool include_all)
{
	int ret;
	char *tracking_dir = NULL;

	ret = create_cache_dirs();
	if (ret) {
		return ret;
	}

	// the data directories are relative to the system
	ret = create_data_dirs(include_all);
	if (ret) {
		return ret;
	}

	// make sure the tracking directory is not empty, if it is,
	// mark all installed bundles as tracked
	tracking_dir = statedir_get_tracking_dir();
	if (!safeguard_tracking_dir(tracking_dir, WORLD_READABLE)) {
		debug("There was an error accessing the tracking directory %s\n", tracking_dir);
	}
	FREE(tracking_dir);

	return ret;
}

static bool set_state_path(char** state, char *path)
{
	if (!path) {
		error("Statedir shouldn't be NULL\n");
		return false;
	}

	if (path[0] != '/') {
		error("State dir must be a full path starting with '/', not '%c'\n", path[0]);
		return false;
	}

	/* Prevent some disasters: since the state dir can be destroyed and
	 * reconstructed, make sure we never set those by accident and nuke the
	 * system. */
	if (!str_cmp(path, "/") || !str_cmp(path, "/var") || !str_cmp(path, "/usr")) {
		error("Refusing to use '%s' as a state dir because it might be erased first\n", path);
		return false;
	}

	FREE(*state);
	*state = sys_path_join("%s", path);

	return true;
}

bool statedir_dup_set_path(char *path)
{
	return set_state_path(&globals.state_dir_cache, path);
}

bool statedir_set_cache_path(char *path)
{
	return set_state_path(&globals.cache_dir, path);
}

bool statedir_set_data_path(char *path)
{
	return set_state_path(&globals.data_dir, path);
}
