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

#include <sys/types.h>						/* time_t */
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>							/* printf() */
#include <assert.h>
#include <stdarg.h>
#include <syslog.h>

#include "initng_syslog.h"
#include <initng_global.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_static_states.h>
#include <initng_event_hook.h>
#include <initng_static_event_types.h>

INITNG_PLUGIN_MACRO;

static int syslog_print_system_state(s_event * event);
static int syslog_print_status_change(s_event * event);
static void check_syslog(void);
static void initng_log(int prio, const char *owner, const char *format, ...);
static void free_buffert(void);

/* global variable */
int syslog_running;
log_ent log_list;

static void check_syslog(void)
{
	struct stat st;

	if (stat("/dev/log", &st) == 0 && S_ISSOCK(st.st_mode))
	{
		syslog_running = 1;

		/* print out the buffers if any now */
		if (!list_empty(&log_list.list))
		{
			log_ent *current, *safe = NULL;

			while_log_list_safe(current, safe)
			{
				initng_log(current->prio, current->owner, "%s",
						   current->buffert);
				free(current->buffert);
				if (current->owner)
					free(current->owner);
				list_del(&current->list);
				free(current);
			}
			INIT_LIST_HEAD(&log_list.list);
		}
	}
	else
	{
		syslog_running = 0;
	}
}

static void free_buffert(void)
{
	log_ent *current, *safe = NULL;

	/* give syslog a last chance to come alive */
	check_syslog();
	while_log_list_safe(current, safe)
	{
		free(current->buffert);
		if (current->owner)
			free(current->owner);
		list_del(&current->list);
		free(current);
	}
	INIT_LIST_HEAD(&log_list.list);

	/* log directly to syslog from now, even if it might not exist */
	syslog_running = 1;
}

static void initng_log(int prio, const char *owner, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);


	/* if syslog is running, send it directly */
	if (syslog_running == 1)
	{
		/* if owner is set, we have openlog with right owner */
		if (owner)
		{
			closelog();
			openlog(owner, 0, LOG_LOCAL1);

			vsyslog(prio, format, ap);

			closelog();
			openlog("InitNG", 0, LOG_LOCAL1);
		}
		else
			vsyslog(prio, format, ap);

	}
	else
	{
		char b[201];
		log_ent *tmp = (log_ent *) i_calloc(1, sizeof(log_ent));

		if (!tmp)
			return;

		tmp->prio = prio;
		vsnprintf(b, 200, format, ap);
		tmp->buffert = i_strdup(b);

		if (owner)
			tmp->owner = i_strdup(owner);
		else
			tmp->owner = NULL;

		list_add(&tmp->list, &log_list.list);

	}

	va_end(ap);
}

/* add values to syslog database */
static int syslog_print_status_change(s_event * event)
{
	active_db_h * service;

	assert(event);
	assert(event->event_type == &EVENT_IS_CHANGE);
	assert(event->data);

	service = event->data;

	assert(service->name);

	if (IS_UP(service))
	{
		check_syslog();
		initng_log(LOG_NOTICE, NULL, "Service %s is up.\n", service->name);
		return (TRUE);
	}

	if (IS_DOWN(service))
	{
		initng_log(LOG_NOTICE, NULL, "Service %s has been stopped.\n",
				   service->name);
		return (TRUE);
	}
	if (IS_FAILED(service))
	{
		initng_log(LOG_NOTICE, NULL, "Service %s FAILED.\n", service->name);
		return (TRUE);
	}

	if (IS_STOPPING(service))
	{
		initng_log(LOG_NOTICE, NULL, "Service %s is stopping.\n",
				   service->name);
		return (TRUE);
	}

	if (IS_STARTING(service))
	{
		initng_log(LOG_NOTICE, NULL, "Service %s is starting.\n",
				   service->name);
		return (TRUE);
	}

	/* leave */
	return (TRUE);
}

static int syslog_print_system_state(s_event * event)
{
	h_sys_state * state;

	assert(event);
	assert(event->event_type == &EVENT_SYSTEM_CHANGE);
	assert(event->data);

	state = event->data;

	switch (*state)
	{
		case STATE_UP:
			initng_log(LOG_NOTICE, NULL, "System is up and running!\n");
			/*
			 * if syslogd have not been started yet, its no ida,
			 * to spend memory for the buffert anyway.
			 */
			free_buffert();
			return (TRUE);
		case STATE_STARTING:
			initng_log(LOG_NOTICE, NULL, "System is starting up.\n");
			return (TRUE);
		case STATE_STOPPING:
			initng_log(LOG_NOTICE, NULL, "System is going down.\n");
			return (TRUE);
		case STATE_ASE:
			initng_log(LOG_NOTICE, NULL, "Last service exited.\n");
			return (TRUE);
		case STATE_EXIT:
			initng_log(LOG_NOTICE, NULL, "Initng is exiting.\n");
			return (TRUE);
		case STATE_RESTART:
			initng_log(LOG_NOTICE, NULL, "Initng is restarting.\n");
			return (TRUE);
		case STATE_HALT:
			initng_log(LOG_NOTICE, NULL, "System is halting.\n");
			return (TRUE);
		case STATE_POWEROFF:
			initng_log(LOG_NOTICE, NULL, "System is power-off.\n");
			return (TRUE);
		case STATE_REBOOT:
			initng_log(LOG_NOTICE, NULL, "System is rebooting.\n");
			return (TRUE);
		default:
			return (TRUE);
	}
	return (TRUE);
}

static int syslog_fetch_output(active_db_h * service, process_h * process,
							   pipe_h * pi, char *buffer_pos)
{
	char log[201];
	int pos = 0;
	int i;

	assert(service);
	assert(service->name);

	/* print every line, ending with a '\n' as an own syslog */
	while (buffer_pos[pos])
	{
		i = 0;
		/* count the number of char before '\n' */
		while (buffer_pos[pos + i] && buffer_pos[pos + i] != '\n' && i < 200)
			i++;

		/* copy that many chars to our temporary log array */
		strncpy(log, &buffer_pos[pos], i);
		log[i] = '\0';

		/* send it to syslog */
		initng_log(LOG_NOTICE, service->name, "%s", log);

		/* step forward */
		pos += i;

		/* and skip the newline if any */
		if (buffer_pos[pos])
			pos++;

	}
	return (TRUE);
}


static int syslog_print_error(e_mt mt, const char *file, const char *func,
							  int line, const char *format, va_list arg)
{
	assert(file);
	assert(func);
	assert(format);
	char tempspace[200];

	vsnprintf(tempspace, 200, format, arg);

	switch (mt)
	{
		case MSG_FAIL:
#ifdef DEBUG
			syslog(LOG_EMERG, "\"%s\", %s() #%i FAIL: %s", file, func, line,
				   tempspace);
#else
			syslog(LOG_EMERG, "FAIL: %s", tempspace);
#endif
			return (TRUE);
		case MSG_WARN:
#ifdef DEBUG
			syslog(LOG_WARNING, "\"%s\", %s() #%i WARN: %s", file, func, line,
				   tempspace);
#else
			syslog(LOG_EMERG, "WARN: %s", tempspace);
#endif
			return (TRUE);
		default:
			syslog(LOG_NOTICE, "%s", tempspace);
			return (TRUE);
	}

	/* newer get here */
	return (FALSE);
}



int module_init(int api_version)
{
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	/* Don't clutter syslog in fake mode */
	if (getpid() != 1 || g.i_am != I_AM_INIT)
	{
		D_("Pid is not 1, (%i), or g.i_am_init not set and the syslog plugin won't load when running in fake mode, to prevent cluttering up the log-files.\n", getpid());
		return (TRUE);
	}
	D_("Initializing syslog plugin\n");

	INIT_LIST_HEAD(&log_list.list);
	check_syslog();

	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("InitNG", 0, LOG_LOCAL1);

	initng_event_hook_register(&EVENT_IS_CHANGE, &syslog_print_status_change);
	initng_event_hook_register(&EVENT_SYSTEM_CHANGE, &syslog_print_system_state);
	initng_plugin_hook_register(&g.BUFFER_WATCHER, 100, &syslog_fetch_output);
	initng_plugin_hook_register(&g.ERR_MSG, 50, &syslog_print_error);

	return (TRUE);
}


void module_unload(void)
{
	/* Don't clutter syslog in fake mode */
	if (g.i_am != I_AM_INIT)
	{
		D_("The syslog plugin won't load when running in fake mode, to prevent cluttering up the log-files.\n");
		return;
	}

	initng_event_hook_unregister(&EVENT_IS_CHANGE, &syslog_print_status_change);
	initng_event_hook_unregister(&EVENT_SYSTEM_CHANGE, &syslog_print_system_state);
	initng_plugin_hook_unregister(&g.BUFFER_WATCHER, &syslog_fetch_output);
	initng_plugin_hook_unregister(&g.ERR_MSG, &syslog_print_error);
	free_buffert();
	closelog();
}
