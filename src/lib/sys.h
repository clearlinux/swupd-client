#ifndef __SWUPD_SYS__
#define __SWUPD_SYS__

#ifdef __cplusplus
extern "C" {
#endif

/* Return the free available space in the partition mounted in path */
long get_available_space(const char *path);

/* run_command_quiet: runs a command redirecting the standard and error outputs
 * redirected to /dev/null. */
#define run_command_quiet(...) run_command_full("/dev/null", "/dev/null", __VA_ARGS__)

/* run_command: runs a command with standard and error output (stdout and stderr) set */
#define run_command(...) run_command_full(NULL, NULL, __VA_ARGS__)

/* run_command_full: Run command cmd with parameters informed in a NULL
 * terminated list of strings.
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

/* copy_all: Runs cp -a [src] [dst] using run_command_quiet */
int copy_all(const char *src, const char *dst);

/* copy: Runs cp [src] [dst] using run_command_quiet */
int copy(const char *src, const char *dst);

/* Recursively create a directory and its parent dirs. */
int mkdir_p(const char *dir);

/* Remove a file using /bin/rm -rf */
int rm_rf(const char *file);

#ifdef __cplusplus
}
#endif

#endif
