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
 * @brief Run command cmd with parameters informed in a NULL terminated list of strings.
 *
 * Return a negative number (-errno) if we are unable to create a new thread to
 * run the command and a positive exit code (0-255) if the command was executed.
 * If there's an error in the execution of the program 255 may also be returned
 *
 * Notes:
 * - This function doesn't execute on a shell like system() so we won't have any
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
 * @brief Return a list of all files in a directory, sorted lexicographically
 */
struct list *get_dir_files_sorted(char *path);

/**
 * @brief Checks if a file exists in the filesystem.
 */
bool file_exists(const char *filename);

/**
 * @brief Checks if a file exists in the filesystem and is executable.
 */
bool file_is_executable(const char *filename);

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
 * @brief Join 2 paths using the default path separator.
 *
 * Can be used to prepend a prefix to an path (eg: the global path_prefix to a
 * file->filename or some other path prefix and path), insuring there is no
 * duplicate '/' at the strings' junction and no trailing '/'.
 * Also works to append a path at the end of URIs.
 */
char *sys_path_join(const char *prefix, const char *path);

/**
 * @brief Check if file exists and it's a directory.
 */
bool sys_is_directory(const char *filename);

/**
 * @brief Remove trailing '/' on string.
 * @note String will be altered, so shouldn't be a constant.
 */
void sys_remove_trailing_path_separators(char *path);

/**
 * @brief Check if a path is an absolute path.
 * @returns false if there's any symlink or ../, ./ in the path.
 */
bool sys_is_absolute_path(const char *path);

/**
 * @brief Run a systemctl command with the informed parameters.
 */
#define systemctl_cmd(...) run_command_quiet(SYSTEMCTL, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
