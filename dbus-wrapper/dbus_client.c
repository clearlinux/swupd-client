/*
 *   Software Updater - D-Bus client for the daemon controlling
 *                      Clear Linux Software Update Client.
 *
 *      Copyright © 2016 Intel Corporation.
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
 *   Contact: Dmitry Rozhkov <dmitry.rozhkov@intel.com>
 */

#include <errno.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "dbus_client.h"
#include "option.h"
#include "log.h"

typedef struct _command_ctx {
	sd_bus *bus;
	const char *method;
	struct list *opts;
	dbus_cmd_argv_type argv_type;
	char **argv;
} command_ctx_t;

static int on_child_output_received(sd_bus_message *message, void *userdata, sd_bus_error *error)
{
	const char *output;
	int r;

	r  = sd_bus_message_read(message, "s", &output);
	if (r < 0) {
		ERR("Can't read client's output: %s", strerror(-r));
		return -1;
	}
	printf("%s", output);

	return 0;
}

static int on_request_completed(sd_bus_message *message, void *userdata, sd_bus_error *error)
{
	sd_event *event = userdata;
	const char *method;
	int code;
	int r;

	r  = sd_bus_message_read(message, "si", &method, &code);
	if (r < 0) {
		ERR("Can't read request results: %s", strerror(-r));
		abort();
	}

	r = sd_event_exit(event, code);
	if (r < 0) {
		ERR("Can't exit event loop: %s", strerror(-r));
		abort();
	}

	return 0;
}

static int on_run_command(sd_event_source *s, void *userdata)
{
	command_ctx_t *ctx = userdata;
	struct list *opts = list_head(ctx->opts);
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply = NULL;
	sd_bus_message *m = NULL;
	int return_value = 0;
	int r;

	r = sd_bus_message_new_method_call(ctx->bus, &m,
					   "org.O1.swupdd.Client",
					   "/org/O1/swupdd/Client",
					   "org.O1.swupdd.Client",
					   ctx->method);
	if (r < 0) {
		ERR("Failed to create method call: %s", strerror(-r));
		goto finish;
	}
	r = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
	while (opts) {
		command_option_t *option = opts->data;
		r = sd_bus_message_open_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
		r = sd_bus_message_append_basic(m, 's', option->name);
		switch (option->type) {
		case TYPE_STRING:
			r = sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT, "s");
			r = sd_bus_message_append_basic(m, 's', option->value.as_str);
			break;
		case TYPE_BOOL:
			r = sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT, "b");
			r = sd_bus_message_append_basic(m, 'b', &option->value.as_bool);
			break;
		case TYPE_INT:
			r = sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT, "i");
			r = sd_bus_message_append_basic(m, 'i', &option->value.as_int);
			break;
		default:
			ERR("Unknown type");
			abort();
		}
		r = sd_bus_message_close_container(m); /* SD_BUS_TYPE_VARIANT */
		if (r < 0) {
			ERR("Failed to close variant container: %s", strerror(-r));
			goto finish;
		}
		r = sd_bus_message_close_container(m); /* SD_BUS_TYPE_DICT_ENTRY */
		if (r < 0) {
			ERR("Failed to close dict entry container: %s", strerror(-r));
			goto finish;
		}
		opts = opts->next;
	}
	r = sd_bus_message_close_container(m); /* SD_BUS_TYPE_ARRAY */
	if (r < 0) {
		ERR("Failed to close array container: %s", strerror(-r));
		goto finish;
	}

	if (ctx->argv_type == DBUS_CMD_SINGLE_ARG) {
		r = sd_bus_message_append_basic(m, 's', ctx->argv[0] ? ctx->argv[0] : "");
		if (r < 0) {
			ERR("Failed to append argument: %s", strerror(-r));
			goto finish;
		}
	} else if (ctx->argv_type == DBUS_CMD_MULTIPLE_ARGS) {
		r = sd_bus_message_append_strv(m, ctx->argv);
		if (r < 0) {
			ERR("Failed to append arguments: %s", strerror(-r));
			goto finish;
		}
	}

	r = sd_bus_call(ctx->bus, m, 0, &error, &reply);
	if (r < 0) {
		ERR("Failed to make D-Bus call: %s (%s)",
		    strerror(sd_bus_error_get_errno(&error)),
		    error.message);
		goto finish;
	}

	/* TODO: do we really need return_value? */
	r = sd_bus_message_read(reply, "b", &return_value);
	if (r < 0) {
		ERR("Failed to parse response message: %s", strerror(-r));
		goto finish;
	}

	if (!return_value) {
		ERR("Hm... something's wrong if there's no D-Bus error set");
	}

finish:
	sd_bus_message_unref(reply);
	sd_bus_message_unref(m);
	sd_bus_error_free(&error);
	if (r < 0) {
		sd_event *event;
		r = sd_event_default(&event);
		if (r < 0) {
			ERR("Failed to get event loop: %s", strerror(-r));
			abort();
		}
		sd_event_exit(event, 1);
		sd_event_unref(event);
	}
	return 0;
}

static int on_command_cancel(sd_event_source *s, const struct signalfd_siginfo *si, void *userdata)
{
	sd_bus *bus = userdata;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply;
	int success = 1;
	int r;

	r = sd_bus_call_method(bus,
			       "org.O1.swupdd.Client",
			       "/org/O1/swupdd/Client",
			       "org.O1.swupdd.Client",
			       "Cancel",
			       &error,
			       &reply,
			       "b", 0);
	if (r < 0) {
		ERR("Failed to call D-Bus method Cancel: %s", strerror(-r));
		goto finish;
	}

	r = sd_bus_message_read(reply, "b", &success);
	if (r < 0) {
		ERR("Failed to parse reply: %s", strerror(-r));
		goto finish;
	}
finish:
	if (r < 0 || !success) {
		ERR("Failed to cancel. Not running?");
		sd_event *event;
		r = sd_event_default(&event);
		if (r < 0) {
			ERR("Failed to get event loop: %s", strerror(-r));
			abort();
		}
		sd_event_exit(event, 1);
		sd_event_unref(event);
	}
	sd_bus_error_free(&error);
	sd_bus_message_unref(reply);
	return 0;
}

int dbus_client_call_method(const char *const method, struct list *opts, dbus_cmd_argv_type argv_type,
			    char *argv[])
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus *bus = NULL;
	sd_event *event;
	sigset_t ss;
	int r = 0;

	r = sd_event_default(&event);
	if (r < 0) {
		ERR("Failed to create event loop: %s", strerror(-r));
		goto finish;
	}

	if (sigemptyset(&ss) < 0 ||
	    sigaddset(&ss, SIGTERM) < 0 ||
	    sigaddset(&ss, SIGINT) < 0) {
		r = -errno;
		ERR("Failed to initialize signal set: %s", strerror(-r));
		goto finish;
	}

	if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0) {
		r = -errno;
		ERR("Failed to block signal set: %s", strerror(-r));
		goto finish;
	}

	r = sd_bus_open_system(&bus);
	if (r < 0) {
		ERR("Failed to connect to system bus: %s", strerror(-r));
		goto finish;
	}

	r = sd_event_add_signal(event, NULL, SIGTERM, on_command_cancel, bus);
	if (r < 0) {
		ERR("Failed to add signal handler: %s", strerror(-r));
		goto finish;
	}
	r = sd_event_add_signal(event, NULL, SIGINT, on_command_cancel, bus);
	if (r < 0) {
		ERR("Failed to add signal handler: %s", strerror(-r));
		goto finish;
	}

	r = sd_bus_add_match(bus,
			     NULL, /* bus slot */
			     "type='signal',"
			     "interface='org.O1.swupdd.Client',"
			     "member='ChildOutputReceived',"
			     "path='/org/O1/swupdd/Client'",
			     on_child_output_received,
			     NULL /* user data */);
	if (r < 0) {
		ERR("Failed to add handler for ChildOutputReceived signal: %s", strerror(-r));
		goto finish;
	}
	r = sd_bus_add_match(bus,
			     NULL, /* bus slot */
			     "type='signal',"
			     "interface='org.O1.swupdd.Client',"
			     "member='RequestCompleted',"
			     "path='/org/O1/swupdd/Client'",
			     on_request_completed,
			     event /* user data */);
	if (r < 0) {
		ERR("Failed to add handler for ChildOutputReceived signal: %s", strerror(-r));
		goto finish;
	}

        r = sd_bus_attach_event(bus, event, 0);
        if (r < 0) {
                ERR("Failed to attach bus to event loop: %s", strerror(-r));
		goto finish;
	}

	command_ctx_t ctx = {
		.bus = bus,
		.method = method,
		.opts = opts,
		.argv_type = argv_type,
		.argv = argv
	};
	r = sd_event_add_defer(event, NULL, on_run_command, &ctx);
	if (r < 0) {
		ERR("Can't schedule command: %s", strerror(-r));
		goto finish;
	}

	if (sd_event_loop(event)) {
		r = -1;
	}

finish:
	sd_bus_error_free(&error);
	sd_bus_unref(bus);
	sd_event_unref(event);

	return r;
}
