/*
 * Daemon for controlling Clear Linux Software Update Client
 *
 * Copyright (C) 2016 Intel Corporation
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
 * Contact: Dmitry Rozhkov <dmitry.rozhkov@intel.com>
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-daemon.h>

#include "log.h"
#include "list.h"

#define SWUPD_CLIENT    "swupd"
#define TIMEOUT_EXIT_SEC 30

typedef enum {
	METHOD_NOTSET = 0,
	METHOD_CHECK_UPDATE,
	METHOD_UPDATE,
	METHOD_VERIFY,
	METHOD_BUNDLE_ADD,
	METHOD_BUNDLE_REMOVE,
	METHOD_HASH_DUMP,
	METHOD_SEARCH
} method_t;

typedef struct _daemon_state {
	sd_bus *bus;
	pid_t child;
	method_t method;
} daemon_state_t;

static const char * const _method_str_map[] = {
	NULL,
	"CheckUpdate",
	"Update",
	"Verify",
	"BundleAdd",
	"BundleRemove",
	"HashDump",
	"Search"
};

static const char * const _method_opt_map[] = {
	NULL,
	"check-update",
	"update",
	"verify",
	"bundle-add",
	"bundle-remove",
	"hashdump",
	"search"
};

static char **list_to_strv(struct list *strlist)
{
	char **strv;
	char **temp;

	strv = (char **)calloc((list_len(strlist) + 1), sizeof(char *));

	temp = strv;
	while (strlist)
	{
		*temp = strlist->data;
		temp++;
		strlist = strlist->next;
	}
	return strv;
}

static int is_in_array(const char *key, char const * const arr[])
{
	if (arr == NULL) {
		return 0;
	}

	char const * const *temp = arr;
	while (*temp) {
		if (strcmp(key, *temp) == 0) {
			return 1;
		}
		temp++;
	}
	return 0;
}

static int bus_message_read_option_string(sd_bus_message *m,
					  const char *optname,
					  struct list **strlist,
					  sd_bus_error *error)
{
	int r = 0;
	char *option = NULL;
	const char *value;

	r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Failed to enter variant container of '%s'", optname);
		return r;
	}
	r = sd_bus_message_read(m, "s", &value);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't read value of '%s'", optname);
		return r;
	}
	r = sd_bus_message_exit_container(m);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't exit variant container of '%s'", optname);
		return r;
	}

	r = asprintf(&option, "--%s", optname);
	if (r < 0) {
		sd_bus_error_set_errnof(error, ENOMEM, "Can't allocate memory for '%s'", optname);
		return -ENOMEM;
	}

	*strlist = list_append_data(*strlist, option);
	*strlist = list_append_data(*strlist, strdup(value));

	return 0;
}

static int bus_message_read_option_bool(sd_bus_message *m,
					const char *optname,
					struct list **strlist,
					sd_bus_error *error)
{
	int value;
	int r;

	r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "b");
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Failed to enter variant container of '%s'", optname);
		return r;
	}
	r = sd_bus_message_read(m, "b", &value);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't read value of '%s'", optname);
		return r;
	}
	r = sd_bus_message_exit_container(m);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't exit variant container of '%s'", optname);
		return r;
	}

	if (value) {
		char *option = NULL;

		r = asprintf(&option, "--%s", optname);
		if (r < 0) {
			sd_bus_error_set_errnof(error, ENOMEM, "Can't allocate memory for '%s'", optname);
			return -ENOMEM;
		}
		*strlist = list_append_data(*strlist, option);
	}
	return 0;
}

static int bus_message_read_option_int(sd_bus_message *m,
				       const char *optname,
				       struct list **strlist,
				       sd_bus_error *error)
{
	char *option = NULL;
	char *value_str = NULL;
	int value;
	int r;

	r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "i");
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Failed to enter variant container of '%s'", optname);
		return r;
	}
	r = sd_bus_message_read(m, "i", &value);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't read value of '%s'", optname);
		return r;
	}
	r = sd_bus_message_exit_container(m);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't exit variant container of '%s'", optname);
		return r;
	}

	if (asprintf(&option, "--%s", optname) < 0) {
		sd_bus_error_set_errnof(error, ENOMEM, "Can't allocate memory for '%s'", optname);
		return -ENOMEM;
	}
	*strlist = list_append_data(*strlist, option);

	if (asprintf(&value_str, "%i", value) < 0) {
		sd_bus_error_set_errnof(error, ENOMEM, "Can't allocate memory for '%i'", value);
		return -ENOMEM;
	}
	*strlist = list_append_data(*strlist, value_str);

	return 0;
}

static int bus_message_read_options(sd_bus_message *m,
	                            char const * const opts_str[],
	                            char const * const opts_bool[],
				    char const * const opts_int[],
				    struct list **strlist,
				    sd_bus_error *error)
{
	int r;

	r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Failed to enter options container");
		return r;
	}

	while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
                const char *argname;

                r = sd_bus_message_read(m, "s", &argname);
                if (r < 0) {
			sd_bus_error_set_errnof(error, -r, "Can't read option name");
                        return r;
		}
		if (is_in_array(argname, opts_str)) {
			r = bus_message_read_option_string(m, argname, strlist, error);
			if (r < 0) {
				return r;
			}
		} else if (is_in_array(argname, opts_bool)) {
			r = bus_message_read_option_bool(m, argname, strlist, error);
			if (r < 0) {
				return r;
			}
		} else if (is_in_array(argname, opts_int)) {
			r = bus_message_read_option_int(m, argname, strlist, error);
			if (r < 0) {
				return r;
			}
		} else {
			r = sd_bus_message_skip(m, "v");
			if (r < 0) {
				sd_bus_error_set_errnof(error, -r, "Can't skip unwanted option value");
				return r;
			}
		}

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
			sd_bus_error_set_errnof(error, -r, "Can't exit dict entry container");
                        return r;
		}
	}
	r = sd_bus_message_exit_container(m);
	if (r < 0) {
		sd_bus_error_set_errnof(error, -r, "Can't exit options container");
		return r;
	}

	return 0;
}

static int on_name_owner_change(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	sd_event *e = userdata;

	assert(m);
	assert(e);

	sd_bus_close(sd_bus_message_get_bus(m));
	sd_event_exit(e, 0);

	return 1;
}

static int on_childs_output(sd_event_source *s, int fd, uint32_t revents, void *userdata)
{
	daemon_state_t *context = userdata;
	int r = 0;
	char buffer[PIPE_BUF + 1];
	ssize_t count;

	if ((revents & EPOLLHUP) == EPOLLHUP) {
		goto finish;
	}

	while ((count = read(fd, buffer, PIPE_BUF)) < 0 && (errno == EINTR)) {}
	if (count > 0) {
		buffer[count] = '\0';
		r = sd_bus_emit_signal(context->bus,
				       "/org/O1/swupdd/Client",
				       "org.O1.swupdd.Client",
				       "ChildOutputReceived", "s", buffer);
		if (r < 0) {
			ERR("Failed to emit signal: %s", strerror(-r));
		}

		return 0;
	} else if (count < 0) {
		ERR("Failed to read pipe: %s", strerror(errno));
		/* Disable the handler since the pipe is broken */
		r = -1;
	}

finish:
	/* No more events for this handler are expected */
	close(fd);
	sd_event_source_unref(s);
	return r;
}

static int on_child_exit(sd_event_source *s, const struct signalfd_siginfo *si, void *userdata)
{
	daemon_state_t *context = userdata;
	int child_exit_status;
	int status = 0;
	int r;

	method_t child_method = context->method;
	assert(child_method);

	if (si->ssi_code == CLD_EXITED) {
		status = si->ssi_status;
	} else {
		DEBUG("Child process was killed");
		/* si->ssi_status is signal code in this case */
		status = 128 + si->ssi_status;
	}

	/* Reap the zomby */
	assert(si->ssi_pid == context->child);
	pid_t ws = waitpid(si->ssi_pid, &child_exit_status, 0);
	assert(ws != -1);

	context->child = 0;
	context->method = METHOD_NOTSET;

	r = sd_bus_emit_signal(context->bus,
			       "/org/O1/swupdd/Client",
			       "org.O1.swupdd.Client",
			       "RequestCompleted", "si", _method_str_map[child_method], status);
	if (r < 0) {
		ERR("Can't emit D-Bus signal: %s", strerror(-r));
	}

	return 0;
}

static int run_swupd(method_t method, struct list *args, daemon_state_t *context)
{
	pid_t pid;
	int fdm, fds;
	int r;

	fdm = posix_openpt(O_RDONLY|O_NOCTTY);
	if (fdm < 0) {
		ERR("Can't open pseudo terminal: %s", strerror(errno));
		return -errno;
	}
	r = grantpt(fdm);
	if (r != 0) {
		ERR("Can't grant pseudo terminal: %s", strerror(errno));
		return -errno;
	}
	r = unlockpt(fdm);
	if (r != 0) {
		ERR("Can't unlock pseudo terminal: %s", strerror(errno));
		return -errno;
	}

	fds = open(ptsname(fdm), O_WRONLY);
	if (fds < 0) {
		ERR("Can't open slave pseudo terminal: %s", strerror(errno));
		return -errno;
	}

	pid = fork();

	if (pid == 0) { /* child */
		struct termios old_term_settings;
		struct termios new_term_settings;
		r = tcgetattr(fds, &old_term_settings);
		if (r < 0) {
			ERR("Can't get slave term settings: %s", strerror(errno));
			_exit(1);
		}
		new_term_settings = old_term_settings;
		cfmakeraw(&new_term_settings);
		tcsetattr(fds, TCSANOW, &new_term_settings);
		while ((dup2(fds, STDOUT_FILENO) == -1) && (errno == EINTR)) {}
		while ((dup2(fds, STDERR_FILENO) == -1) && (errno == EINTR)) {}
		close(fds);
		close(fdm);
		char **argv = list_to_strv(list_head(args));
		execvp(*argv, argv);
		ERR("This line must not be reached");
		_exit(1);
	} else if (pid < 0) {
		ERR("Failed to fork: %s", strerror(errno));
		close(fds);
		close(fdm);
		return -errno;
	}

	close(fds);
	context->child = pid;
	context->method = method;
	sd_event *event = NULL;
	r = sd_event_default(&event);
	r = sd_event_add_io(event, NULL, fdm, EPOLLIN, on_childs_output, context);
	assert(r >= 0);
	sd_event_unref(event);

	return 0;
}

static int check_prerequisites(daemon_state_t *context, sd_bus_error *error)
{
	if (context->child) {
		sd_bus_error_set_errnof(error, EAGAIN, "Busy with ongoing request to swupd");
		return -EAGAIN;
	}

	return 0;
}

static int method_update(sd_bus_message *m,
	                 void *userdata,
	                 sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_UPDATE]));

	char const * const str_opts[] = {"url", "contenturl", "versionurl",
					 "format", "statedir", "path", NULL};
	char const * const bool_opts[] = {"download", "status", "force", NULL};
	char const * const int_opts[] = {"port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	r  = run_swupd(METHOD_UPDATE, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_verify(sd_bus_message *m,
	                 void *userdata,
	                 sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_VERIFY]));

	char const * const str_opts[] = {"path", "url", "contenturl", "versionurl",
					 "format", "statedir", NULL};
	char const * const bool_opts[] = {"fix", "install", "quick", "force", NULL};
	char const * const int_opts[] = {"manifest", "port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	r  = run_swupd(METHOD_VERIFY, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_check_update(sd_bus_message *m,
			       void *userdata,
			       sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_CHECK_UPDATE]));

	char const * const str_opts[] = {"url", "versionurl", "format", "statedir", "path", NULL};
	char const * const bool_opts[] = {"force", NULL};
	char const * const int_opts[] = {"port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	const char *bundle;
	r = sd_bus_message_read(m, "s", &bundle);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read bundle");
		goto finish;
	}
	args = list_append_data(args, strdup(bundle));

	r  = run_swupd(METHOD_CHECK_UPDATE, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_hash_dump(sd_bus_message *m,
			    void *userdata,
			    sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_HASH_DUMP]));

	char const * const str_opts[] = {"basepath", NULL};
	char const * const bool_opts[] = {"no-xattrs", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, NULL, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	const char *filename;
	r = sd_bus_message_read(m, "s", &filename);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read file name");
		goto finish;
	}
	args = list_append_data(args, strdup(filename));

	r  = run_swupd(METHOD_HASH_DUMP, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_search(sd_bus_message *m,
			 void *userdata,
			 sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_SEARCH]));

	char const * const str_opts[] = {"url", "contenturl", "versionurl",
					 "path", "scope", "format", "statedir", NULL};
	char const * const bool_opts[] = {"library", "binary", "init",
					  "display-files", NULL};
	char const * const int_opts[] = {"port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	const char *filename;
	r = sd_bus_message_read(m, "s", &filename);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read file name");
		goto finish;
	}
	args = list_append_data(args, strdup(filename));

	r  = run_swupd(METHOD_SEARCH, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_bundle_add(sd_bus_message *m,
			     void *userdata,
			     sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;
	const char* bundle = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_BUNDLE_ADD]));

	char const * const str_opts[] = {"url", "contenturl", "versionurl",
					 "path", "format", "statedir", NULL};
	char const * const bool_opts[] = {"list", "force", NULL};
	char const * const int_opts[] = {"port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s");
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't enter bundles container");
		goto finish;
	}
	while ((r = sd_bus_message_read(m, "s", &bundle)) > 0) {
		args = list_append_data(args, strdup(bundle));
	}
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read bundle name");
		goto finish;
	}
	r = sd_bus_message_exit_container(m);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't exit bundles container");
		goto finish;
	}

	r  = run_swupd(METHOD_BUNDLE_ADD, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_bundle_remove(sd_bus_message *m,
				void *userdata,
				sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	struct list *args = NULL;

	r = check_prerequisites(context, ret_error);
	if (r < 0) {
		return r;
	}

	args = list_append_data(args, strdup(SWUPD_CLIENT));
	args = list_append_data(args, strdup(_method_opt_map[METHOD_BUNDLE_REMOVE]));

	char const * const str_opts[] = {"path", "url", "contenturl", "versionurl",
					 "format", "statedir", NULL};
	char const * const bool_opts[] = {"force", NULL};
	char const * const int_opts[] = {"port", NULL};
	r = bus_message_read_options(m, str_opts, bool_opts, int_opts, &args, ret_error);
	if (r < 0) {
		goto finish;
	}

	const char *bundle;
	r = sd_bus_message_read(m, "s", &bundle);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read bundle name");
		goto finish;
	}
	args = list_append_data(args, strdup(bundle));

	r  = run_swupd(METHOD_BUNDLE_REMOVE, args, context);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Failed to run swupd command");
		goto finish;
	}

	r = sd_bus_reply_method_return(m, "b", (r >= 0));

finish:
	list_free_list_and_data(args, free);
	return r;
}

static int method_cancel(sd_bus_message *m,
			 void *userdata,
			 sd_bus_error *ret_error)
{
	daemon_state_t *context = userdata;
	int r = 0;
	pid_t child;
	int force;

	child = context->child;
	if (!child) {
		sd_bus_error_set_errnof(ret_error, ECHILD, "No child process to cancel");
		return -ECHILD;
	}

	r = sd_bus_message_read(m, "b", &force);
	if (r < 0) {
		sd_bus_error_set_errnof(ret_error, -r, "Can't read 'force' option");
		return r;
	}

	if (force) {
		kill(child, SIGKILL);
	} else {
		kill(child, SIGTERM);
	}

	return sd_bus_reply_method_return(m, "b", (r >= 0));
}

/* Basically this is a copypasta from systemd's internal function bus_event_loop_with_idle() */
static int run_bus_event_loop(sd_event *event,
			      daemon_state_t *context)
{
	sd_bus *bus = context->bus;
	bool exiting = false;
	int r, code;

	for (;;) {
		r = sd_event_get_state(event);
		if (r < 0) {
			ERR("Failed to get event loop's state: %s", strerror(-r));
			return r;
		}
		if (r == SD_EVENT_FINISHED) {
			break;
		}

		r = sd_event_run(event, exiting ? (uint64_t) -1 : (uint64_t) TIMEOUT_EXIT_SEC * 1000000);
		if (r < 0) {
			ERR("Failed to run event loop: %s", strerror(-r));
			return r;
		}

		if (!context->method && r == 0 && !exiting) {
			r = sd_bus_try_close(bus);
			if (r == -EBUSY) {
				continue;
			}
                        /* Fallback for dbus1 connections: we
                         * unregister the name and wait for the
                         * response to come through for it */
                        if (r == -EOPNOTSUPP) {
				sd_notify(false, "STOPPING=1");

				/* We unregister the name here and then wait for the
				 * NameOwnerChanged signal for this event to arrive before we
				 * quit. We do this in order to make sure that any queued
				 * requests are still processed before we really exit. */

				char *match = NULL;
				const char *unique;
				r = sd_bus_get_unique_name(bus, &unique);
				if (r < 0) {
					ERR("Failed to get unique D-Bus name: %s", strerror(-r));
					return r;
				}
				r = asprintf(&match,
					     "sender='org.freedesktop.DBus',"
					     "type='signal',"
					     "interface='org.freedesktop.DBus',"
					     "member='NameOwnerChanged',"
					     "path='/org/freedesktop/DBus',"
					     "arg0='org.O1.swupdd.Client',"
					     "arg1='%s',"
					     "arg2=''", unique);
				if (r < 0) {
					ERR("Not enough memory to allocate string");
					return r;
				}

				r = sd_bus_add_match(bus, NULL, match, on_name_owner_change, event);
				if (r < 0) {
					ERR("Failed to add signal listener: %s", strerror(-r));
					free(match);
					return r;
				}

				r = sd_bus_release_name(bus, "org.O1.swupdd.Client");
				if (r < 0) {
					ERR("Failed to release service name: %s", strerror(-r));
					free(match);
					return r;
				}

				exiting = true;
				free(match);
				continue;
			}

			if (r < 0) {
				ERR("Failed to close bus: %s", strerror(-r));
				return r;
			}
			sd_event_exit(event, 0);
			break;
		}
	}

        r = sd_event_get_exit_code(event, &code);
        if (r < 0) {
                return r;
	}

        return code;
}

static const sd_bus_vtable swupdd_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("CheckUpdate", "a{sv}s", "b", method_check_update, 0),
	SD_BUS_METHOD("HashDump", "a{sv}s", "b", method_hash_dump, 0),
	SD_BUS_METHOD("Search", "a{sv}s", "b", method_search, 0),
	SD_BUS_METHOD("Update", "a{sv}", "b", method_update, 0),
	SD_BUS_METHOD("Verify", "a{sv}", "b", method_verify, 0),
	SD_BUS_METHOD("BundleAdd", "a{sv}as", "b", method_bundle_add, 0),
	SD_BUS_METHOD("BundleRemove", "a{sv}s", "b", method_bundle_remove, 0),
	SD_BUS_METHOD("Cancel", "b", "b", method_cancel, 0),
	SD_BUS_SIGNAL("RequestCompleted", "si", 0),
	SD_BUS_SIGNAL("ChildOutputReceived", "s", 0),
	SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
	daemon_state_t context;
	sd_bus_slot *slot = NULL;
	sd_event *event = NULL;
	sigset_t ss;
	int r;

	memset(&context, 0x00, sizeof(daemon_state_t));

        r = sd_event_default(&event);
        if (r < 0) {
                ERR("Failed to allocate event loop: %s", strerror(-r));
                goto finish;
        }

	if (sigemptyset(&ss) < 0 ||
	    sigaddset(&ss, SIGCHLD) < 0) {
		r = -errno;
		goto finish;
	}

	if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0) {
		ERR("Failed to set signal proc mask: %s", strerror(errno));
		goto finish;
	}
	r = sd_event_add_signal(event, NULL, SIGCHLD, on_child_exit, &context);
	if (r < 0) {
		ERR("Failed to add signal: %s", strerror(-r));
		goto finish;
	}

        sd_event_set_watchdog(event, true);

	r = sd_bus_open_system(&context.bus);
	if (r < 0) {
		ERR("Failed to connect to system bus: %s", strerror(-r));
		goto finish;
	}

	r = sd_bus_add_object_vtable(context.bus,
				     &slot,
				     "/org/O1/swupdd/Client",
				     "org.O1.swupdd.Client",
				     swupdd_vtable,
				     NULL);
	if (r < 0) {
		ERR("Failed to issue method call: %s", strerror(-r));
		goto finish;
	}

	sd_bus_slot_set_userdata(slot, &context);

	r = sd_bus_request_name(context.bus, "org.O1.swupdd.Client", 0);
	if (r < 0) {
		ERR("Failed to acquire service name: %s", strerror(-r));
		goto finish;
	}

        r = sd_bus_attach_event(context.bus, event, 0);
        if (r < 0) {
                ERR("Failed to attach bus to event loop: %s", strerror(-r));
		goto finish;
	}

	sd_notify(false,
		  "READY=1\n"
		  "STATUS=Daemon startup completed, processing events.");
	r = run_bus_event_loop(event, &context);

finish:
	sd_bus_slot_unref(slot);
	sd_bus_unref(context.bus);
	sd_event_unref(event);

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
