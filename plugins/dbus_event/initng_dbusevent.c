/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <initng.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

#include <initng_global.h>
#include <initng_active_state.h>
#include <initng_active_db.h>
#include <initng_process_db.h>
#include <initng_service_cache.h>
#include <initng_handler.h>
#include <initng_active_db.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_load_module.h>
#include <initng_plugin_callers.h>
#include <initng_error.h>
#include <initng_plugin.h>
#include <initng_static_states.h>
#include <initng_control_command.h>

#include <initng-paths.h>
#include "initng_dbusevent.h"

INITNG_PLUGIN_MACRO;

void send_to_all(char *buf);


static int astatus_change(active_db_h * service);
static void check_socket(int signal);
static int connect_to_dbus(void);
static void system_state_change(e_is state);
static int system_pipe_watchers(active_db_h * service, process_h * process,
								pipe_h * pi, char *output);
static int print_error(e_mt mt, const char *file, const char *func, int line,
					   const char *format, va_list arg);

static dbus_bool_t add_dbus_watch(DBusWatch * watch, void *data);
static void rem_dbus_watch(DBusWatch * watch, void *data);
static void toggled_dbus_watch(DBusWatch * watch, void *data);
static void fdw_callback(f_module_h * from, e_fdw what);

static void free_dbus_watch_data(void *data);

DBusConnection *conn;

typedef struct initng_dbus_watch
{
	f_module_h fdw;
	DBusWatch *dbus;
} initng_dbus_watch;

/* ------  DBus Watch Handling --------

   NOTE: if any of the F_, D_ etc macros are called during execution of these
   functions, bad things happen as these call DBus functions and the DBus
   code is *not* reentrant */

static dbus_bool_t add_dbus_watch(DBusWatch * watch, void *data)
{
	initng_dbus_watch *w = i_calloc(1, sizeof(initng_dbus_watch));

	if (w == NULL)
	{
		printf("Memory allocation failed\n");
		return FALSE;
	}

	w->fdw.fds = dbus_watch_get_fd(watch);
	w->fdw.call_module = fdw_callback;
	w->dbus = watch;
	w->fdw.what = 0;

	dbus_watch_set_data(watch, w, free_dbus_watch_data);
	toggled_dbus_watch(watch, data);		/* to set initial state */

	initng_plugin_hook_register(&g.FDWATCHERS, 30, &w->fdw);

	return TRUE;
}

static void rem_dbus_watch(DBusWatch * watch, void *data)
{
	/*   initng_dbus_watch *w = dbus_watch_get_data(watch);

	   if(w != NULL) free_dbus_watch_data(w); */
}

static void toggled_dbus_watch(DBusWatch * watch, void *data)
{
	initng_dbus_watch *w = dbus_watch_get_data(watch);

	w->fdw.what = 0;
	if (dbus_watch_get_enabled(watch))
	{
		int flags = dbus_watch_get_flags(watch);

		if (flags & DBUS_WATCH_READABLE)
			w->fdw.what |= FDW_READ;

		if (flags & DBUS_WATCH_WRITABLE)
			w->fdw.what |= FDW_WRITE;

		w->fdw.what |= FDW_ERROR;
	}

}

static void free_dbus_watch_data(void *data)
{
	initng_dbus_watch *w = data;

	assert(w);
	initng_plugin_hook_unregister(&g.FDWATCHERS, &(w->fdw));
	free(w);
}

static void fdw_callback(f_module_h * from, e_fdw what)
{
	initng_dbus_watch *w = (initng_dbus_watch *) from;
	int flgs = 0;

	/* TODO - handle DBUS_WATCH_HANGUP ? */

	if (what & FDW_READ)
		flgs |= DBUS_WATCH_READABLE;

	if (what & FDW_WRITE)
		flgs |= DBUS_WATCH_WRITABLE;

	if (what & FDW_ERROR)
		flgs |= DBUS_WATCH_ERROR;

	dbus_watch_handle(w->dbus, flgs);
}

/* --- End DBus watch handling ---- */

static int astatus_change(active_db_h * service)
{
	DBusMessage *msg;
	dbus_uint32_t serial = 0;

	/* these values will be send */
	const char *service_name = service->name;
	int is = service->current_state->is;
	const char *state_name = service->current_state->state_name;

	if (conn == NULL)
		return (TRUE);

	D_("Sending signal with value \"%.10s\" %i \"%.10s\"\n", service_name, is,
	   state_name);

	/* create a signal & check for errors */
	msg = dbus_message_new_signal(OBJECT,	/* object name of the signal */
								  INTERFACE,	/* interface name of the signal */
								  "astatus_change");	/* name of the signal */
	if (NULL == msg)
	{
		F_("Unable to create ne dbus signal\n");
		return (TRUE);
	}


	/* Append some arguments to the call */
	if (!dbus_message_append_args
		(msg, DBUS_TYPE_STRING, &service_name, DBUS_TYPE_INT32, &is,
		 DBUS_TYPE_STRING, &state_name, DBUS_TYPE_INVALID))
	{
		F_("Unable to append args to dbus signal!\n");
		return (TRUE);
	}


	/* send the message and flush the connection */
	if (!dbus_connection_send(conn, msg, &serial))
	{
		F_("Unable to send dbus signal!\n");
		return (TRUE);
	}
	// dbus_connection_flush(conn);

	D_("Dbus Signal Sent\n");

	/* free the message */
	dbus_message_unref(msg);

	return (TRUE);
}

static void system_state_change(e_is state)
{
	DBusMessage *msg;
	dbus_uint32_t serial = 0;

	if (conn == NULL)
		return;

	/* create a signal & check for errors */
	msg = dbus_message_new_signal(OBJECT,	/* object name of the signal */
								  INTERFACE,	/* interface name of the signal */
								  "system_state_change");	/* name of the signal */
	if (NULL == msg)
	{
		F_("Unable to create new dbus signal\n");
		return;
	}


	/* Append some arguments to the call */
	if (!dbus_message_append_args
		(msg, DBUS_TYPE_INT32, &state, DBUS_TYPE_INVALID))
	{
		F_("Unable to append args to dbus signal!\n");
		return;
	}


	/* send the message and flush the connection */
	if (!dbus_connection_send(conn, msg, &serial))
	{
		F_("Unable to send dbus signal!\n");
		return;
	}
	//dbus_connection_flush(conn);

	/* free the message */
	dbus_message_unref(msg);

	D_("Dbus Signal Sent\n");
	return;
}

static int system_pipe_watchers(active_db_h * service, process_h * process,
								pipe_h * pi, char *output)
{
	DBusMessage *msg;
	dbus_uint32_t serial = 0;
	const char *service_name = service->name;
	const char *process_name = process->pt->name;

	if (conn == NULL)
		return (TRUE);

	/* create a signal & check for errors */
	msg = dbus_message_new_signal(OBJECT,	/* object name of the signal */
								  INTERFACE,	/* interface name of the signal */
								  "system_output");	/* name of the signal */
	if (NULL == msg)
	{
		F_("Unable to create new dbus signal\n");
		return (TRUE);
	}


	/* Append some arguments to the call */
	if (!dbus_message_append_args
		(msg, DBUS_TYPE_STRING, &service_name, DBUS_TYPE_STRING,
		 &process_name, DBUS_TYPE_STRING, &output, DBUS_TYPE_INVALID))
	{
		F_("Unable to append args to dbus signal!\n");
		return (TRUE);
	}


	/* send the message and flush the connection */
	if (!dbus_connection_send(conn, msg, &serial))
	{
		F_("Unable to send dbus signal!\n");
		return (TRUE);
	}
	//dbus_connection_flush(conn);

	/* free the message */
	dbus_message_unref(msg);

	D_("Dbus Signal Sent\n");
	return (TRUE);
}

static int print_error(e_mt mt, const char *file, const char *func,
					   int line, const char *format, va_list arg)
{
	DBusMessage *msg;
	dbus_uint32_t serial = 0;

	if (conn == NULL)
		return (TRUE);

	/* create a signal & check for errors */
	msg = dbus_message_new_signal(OBJECT,	/* object name of the signal */
								  INTERFACE,	/* interface name of the signal */
								  "print_error");	/* name of the signal */
	if (NULL == msg)
	{
		F_("Unable to create new dbus signal\n");
		return (TRUE);
	}

	/* compose the message */
	char *message = i_calloc(1001, sizeof(char));

	vsnprintf(message, 1000, format, arg);

	/* Append some arguments to the call */
	if (!dbus_message_append_args
		(msg, DBUS_TYPE_INT32, &mt, DBUS_TYPE_STRING, &file, DBUS_TYPE_STRING,
		 &func, DBUS_TYPE_INT32, &line, DBUS_TYPE_STRING, &message,
		 DBUS_TYPE_INVALID))
	{
		F_("Unable to append args to dbus signal!\n");
		return (TRUE);
	}


	/* send the message and flush the connection */
	if (!dbus_connection_send(conn, msg, &serial))
	{
		F_("Unable to send dbus signal!\n");
		return (TRUE);
	}
	//dbus_connection_flush(conn);

	/* free the message */
	dbus_message_unref(msg);
	free(message);

	D_("Dbus Signal Sent\n");
	return (TRUE);
}


/*
 * On a SIGHUP, close and reopen the socket.
 */
static void check_socket(int signal)
{
	/* only react on a SIGHUP signal */
	if (signal != SIGHUP)
		return;

	/* close if open */
	if (conn)
	{
		dbus_connection_close(conn);
		conn = NULL;
	}

	/* and open again */
	connect_to_dbus();
}


static int connect_to_dbus(void)
{
	int ret;
	DBusError err;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err))
	{
		F_("Connection Error (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (conn == NULL)
	{
		return (FALSE);
	}


	dbus_connection_set_watch_functions(conn, add_dbus_watch, rem_dbus_watch,
										toggled_dbus_watch, NULL, NULL);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(conn, SOURCE_REQUEST,
								DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

	/* Make sure no error is set */
	if (dbus_error_is_set(&err))
	{
		F_("Name Error (%s)\n", err.message);
		dbus_error_free(&err);
	}

	/*  IF this is set, initng is the owner of initng.signal.source */
	/*if ( ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) { 
	   printf("Could not gain PRIMARY_OWNER of "SOURCE_REQUEST"\n");
	   return(FALSE);
	   } */

	return (TRUE);
}

int module_init(int api_version)
{

	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", INITNG_VERSION, api_version);
		return (FALSE);
	}

	connect_to_dbus();

	/* add the hooks we are monitoring */
	initng_plugin_hook_register(&g.SIGNAL, 50, &check_socket);
	initng_plugin_hook_register(&g.ASTATUS_CHANGE, 50, &astatus_change);
	initng_plugin_hook_register(&g.SWATCHERS, 50, &system_state_change);
	initng_plugin_hook_register(&g.PIPEWATCHERS, 50, &system_pipe_watchers);
	initng_plugin_hook_register(&g.ERR_MSG, 50, &print_error);


	/* return happily */
	return (TRUE);
}




void module_unload(void)
{
	if (conn != NULL)
	{
		dbus_connection_close(conn);
		conn = NULL;
	}

	initng_plugin_hook_unregister(&g.SIGNAL, &check_socket);
	initng_plugin_hook_unregister(&g.ASTATUS_CHANGE, &astatus_change);
	initng_plugin_hook_unregister(&g.SWATCHERS, &system_state_change);
	initng_plugin_hook_unregister(&g.PIPEWATCHERS, &system_pipe_watchers);
	initng_plugin_hook_unregister(&g.ERR_MSG, &print_error);
}
