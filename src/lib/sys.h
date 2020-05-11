#ifndef __SWUPD_SYS__
#define __SWUPD_SYS__

/**
 * @file
 * @brief Functions to call System operations.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Systemctl path in the system.
 */
#define SYSTEMCTL "/usr/bin/systemctl"

/**
 * @brief Character used as PATH separator.
 */
#define PATH_SEPARATOR '/'

/**
 * @brief Return the free available space in the partition mounted in path
 */
long get_available_space(const char *path);

/**
 * @brief Runs a command redirecting the standard and error outputs redirected
 * to /dev/null.
 */
#define run_command_quiet(...) run_command_full("/dev/null", "/dev/null", __VA_ARGS__)

/**
 * @brief runs a command with standard and error output (stdout and stderr) set
 */
#define run_command(...) run_command_full(NULL, NULL, __VA_ARGS__)

/**
 * @brief runs a command from params with standard and error output (stdout and stderr) set
 */
#define run_command_params(_params) run_command_full_params(NULL, NULL, _params)

/**
 * @brief runs a command from params with standard and error output redirected to /dev/null.
 */
#define run_command_params_quiet(_params) run_command_full_params("/dev/null", "/dev/null", _params)

/**
 * @brief Run command cmd with parameters informed in a NULL terminated list of strings.
 *
 * Return a negative number (-errno) if we are unable to create a new thread to
 * run the command and a positive exit code (0-255) if the command was executed.
 * If there's an error in the execution of the program 255 may also be returned
 *
 * Notes:
 * - This function doesn't execute on a shell like system () so we won't have any
 *   issue with parameters globbing.
 * - The full path to cmd is required.
 *
 * Example:
 * run_command(NULL, NULL, "/usr/bin/ls", "/tmp", NULL);
 * This will run "ls /tmp" and redirect output to stdout and stderr.
 */
int run_command_full(const char *stdout_file, const char *stderr_file, const char *cmd, ...);

/**
 * @brief Run command from a NULL terminated string array of parameters.
 *
 * First parameter of the list should be the full path to the
 * command to be executed.
 */
int run_command_full_params(const char *stdout_file, const char *stderr_file, char **params);

/**
 * @brief Runs cp -a [src] [dst] using run_command_quiet.
 */
int copy_all(const char *src, const char *dst);

/**
 * @brief Runs cp [src] [dst] using run_command_quiet.
 */
int copy(const char *src, const char *dst);

/**
 * @brief Recursively create a directory and its parent dirs.
 */
int mkdir_p(const char *dir);

/**
 * @brief Remove a file using /bin/rm -rf
 */
int rm_rf(const char *file);

/**
 * @brief Return a list of files in a directory
 */
struct list *sys_ls(char *path);

/**
 * @brief Checks if a file exists in the filesystem.
 */
bool sys_file_exists(const char *filename);

/**
 * @brief Checks if a file exists in the filesystem following symlinks
 */
bool sys_filelink_exists(const char *filename);

/**
 * @brief Checks if all elements of a path are absolute and don't contain symlinks
 */
bool sys_path_is_absolute(const char *filename);

/**
 * @brief Checks if a file exists in the filesystem and is executable.
 * Follows symlinks
 */
bool sys_filelink_is_executable(const char *filename);

/**
 * @brief Checks if a file is a hardlink to another file
 */
bool sys_file_is_hardlink(const char *file1, const char *file2);

/**
 * @brief Print error 'message' to system journal.
 */
void journal_log_error(const char *message);

/**
 * @brief Restart a systemd service.
 */
int systemctl_restart(const char *service);

/**
 * @brief Restart a systemd service without blocking
 */
int systemctl_restart_noblock(const char *service);

/**
 * @brief Check if systemd is active and running in the system.
 *
 * Necessary because not all commands returns correct error codes when running
 * in a container
 */
bool systemctl_active(void);

/**
 * @brief Re-exec the systemd daemon.
 */
int systemctl_daemon_reexec(void);

/**
 * @brief Reload the systemd daemon.
 */
int systemctl_daemon_reload(void);

/**
 * @brief Check if systemd is running inside a container.
 */
bool systemd_in_container(void);

/**
 * @brief Returns a pointer to a new allocated string with the dirname of the informed
 * filename.
 *
 * Helper to POSIX dirname() function. Using dirname() is tricky because you
 * can't use that with constants and you need to keep the original pointer to
 * free later. Just a wrapper to make it easier to use.
 */
char *sys_dirname(const char *path);

/**
 * @brief Safe GNU implementation of basename
 *
 * Make sure the GNU implementation of basename will be used and not the
 * posix one. The GNU implementation is read-only and won't change the value
 * of path. For more information consult GNU basename manual.
 *
 * A pointer for a portion of the path string will be returned and shoudn't be
 * freed. If you need to keep it you need to strdup it.
 */
char *sys_basename(const char *path);

/**
 * @brief Join N paths using the default path separator.
 *
 * Can be used to prepend a prefix to a path (eg: the global path_prefix to a
 * file->filename or some other path prefix and path), insuring there is no
 * duplicate '/' at the strings' junction.
 * The first argument must be a string that indicates how many
 * arguments are to be concatenated.
 * For example sys_path("%s/%s/%s", path1, path2, path3)
 *
 * @return a string with the path joint.
 */
char *sys_path_join(const char *fmt, ...);

/**
 * @brief Check if current user is root.
 */
bool is_root(void);

/**
 * @brief Check if the provided path is a directory
 */
bool sys_is_dir(const char *path);

/**
 * @brief Check if the provided path is a directory following symlinks
 */
bool sys_filelink_is_dir(const char *path);

/**
 * @brief Remove the content of a directory without removing the directory itself.
 *
 * @return 0 on success, negative value on errors
 */
int sys_rm_dir_contents(const char *path);

/**
 * @brief Remove file or directory.
 *
 * If filename is a directory, removes all files and directories recursively.
 *
 * @return 0 on success, -ENOENT if file isn't present in the system and any
 * negative value on errors
 */
int sys_rm_recursive(const char *filename);

/**
 * @brief Remove a file or an empty directory.
 *
 * If directory isn't empty, function will fail.
 *
 * @return 0 on success, -ENOENT if file isn't present in the system and any
 * negative value on errors
 */
int sys_rm(const char *filename);

/**
 * @brief Get the count of hard links to a file.
 *
 * @return Return the hard link count or a negative number on errors.
 */
long sys_file_hardlink_count(const char *file);

/**
 * @brief Get the size of a file.
 *
 * @return Return the size of a file or a negative number on errors.
 */
long sys_get_file_size(const char *filename);

/**
 * @brief Run a systemctl command with the informed parameters.
 */
#define systemctl_cmd(...) run_command_quiet(SYSTEMCTL, __VA_ARGS__)

/**
 * @brief Run a systemctl command with the informed parameters in a path
 */
#define systemctl_cmd_path(path, ...) \
	path ? run_command_quiet(SYSTEMCTL, "--root", path, __VA_ARGS__) : run_command_quiet(SYSTEMCTL, __VA_ARGS__)

/**
 * @brief function that checks if a directory is empty
 *
 * @return 1 if the directory is empty, 0 if it is not empty, < 0 on errors
 */
int sys_dir_is_empty(const char *path);

/**
 * @brief Writes the specified content into a file
 *
 * @return 0 on success or a value < 0 on error
 */
int sys_write_file(char *file, void *content, size_t content_size);

/**
 * @brief Maps a file into memory.
 *
 * The function maps the file into a buffer and it also stores the length of the data
 * in the file_length
 *
 * @return 0 on successfull mapping or an errno on error
 */
void *sys_mmap_file(const char *file, size_t *file_length);

/**
 * @brief Deletes the mapping for the specified address range
 *
 */
void sys_mmap_free(void *buffer, size_t buffer_length);

/**
 * @brief Try to link a file, if not possible, copy it
 */
int link_or_copy(const char *orig, const char *dest);

/**
 * @brief Try to link a file, if not possible, copy it using copy_all() function
 */
int link_or_copy_all(const char *orig, const char *dest);

/**
 * @brief Try to link a file, if not possible, move it using rename()
 */
int link_or_rename(const char *orig, const char *dest);

#ifdef __cplusplus
}
#endif

#endif
