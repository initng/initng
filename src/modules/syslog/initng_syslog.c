/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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

#include <sys/types.h>		/* time_t */
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>		/* printf() */
#include <assert.h>
#include <stdarg.h>
#include <syslog.h>
#include <signal.h>

#include "initng_syslog.h"

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

static void syslog_print_system_state(s_event * event);
static void syslog_print_status_change(s_event * event);
static void syslog_print_status_killed(s_event * event);
static void check_syslog(void);
static void initng_log(int prio, const char *owner, const char *format, ...);
static void free_buffert(void);

/* global variable */
int syslog_running;
log_ent log_list;

extern void vsyslog(int, const char*, va_list); // FIXME : this isn't standard

/*
 * Forward output to syslog.
 */
s_entry FORWARD_TO_SYSLOG = {
    .name = "forward_to_syslog",
    .description = "Forward output to syslog",
    .type = SET,
    .ot = NULL,
};


static void check_syslog(void)
{
	struct stat st;

	if (stat("/dev/log", &st) == 0 && S_ISSOCK(st.st_mode)) {
		syslog_running = 1;

		/* print out the buffers if any now */
		if (!initng_list_isempty(&log_list.list)) {
			log_ent *current, *safe = NULL;

			while_log_list_safe(current, safe) {
				initng_log(current->prio, current->owner,
					   "%s", current->buffert);
				free(current->buffert);
				free(current->owner);
				initng_list_del(&current->list);
				free(current);
			}
			initng_list_init(&log_list.list);
		}
	} else {
		syslog_running = 0;
	}
}

static void free_buffert(void)
{
	log_ent *current, *safe = NULL;

	/* give syslog a last chance to come alive */
	check_syslog();
	while_log_list_safe(current, safe) {
		free(current->buffert);
		free(current->owner);

		initng_list_del(&current->list);
		free(current);
	}
	initng_list_init(&log_list.list);

	/* log directly to syslog from now, even if it might not exist */
	syslog_running = 1;
}

static void initng_log(int prio, const char *owner, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	/* if syslog is running, send it directly */
	if (syslog_running == 1) {
		/* if owner is set, we have openlog with right owner */
		if (owner) {
			closelog();
			openlog(owner, 0, LOG_LOCAL1);

			vsyslog(prio, format, ap);

			closelog();
			openlog("InitNG", 0, LOG_LOCAL1);
		} else {
			vsyslog(prio, format, ap);
		}
	} else {
		char b[201];
		log_ent *tmp = (log_ent *) initng_toolbox_calloc(1,
							 sizeof(log_ent));

		if (tmp) {
			tmp->prio = prio;
			vsnprintf(b, 200, format, ap);
			tmp->buffert = initng_toolbox_strdup(b);

			if (owner)
				tmp->owner = initng_toolbox_strdup(owner);
			else
				tmp->owner = NULL;

			initng_list_add(&tmp->list, &log_list.list);
		}
	}

	va_end(ap);
}

static void syslog_print_status_killed(s_event * event)
{
	s_event_handle_killed_data * data;
	assert(event->event_type == &EVENT_HANDLE_KILLED);
	assert(event->data);
	data = event->data;
	if (strcmp(data->process->pt->name, "daemon") != 0)
		return;
	if (WIFSIGNALED(data->process->r_code))
	{
		int sig = WTERMSIG(data->process->r_code);
		if (sig != SIGTERM)
		{
			initng_log(LOG_ALERT, NULL, "%s %s has been killed by signal %d !!!",
			    data->process->pt->name, data->service->name, sig);
		}
	}
}

/* add values to syslog database */
static void syslog_print_status_change(s_event * event)
{
	active_db_h *service;

	assert(event->event_type == &EVENT_IS_CHANGE);
	assert(event->data);

	service = event->data;

	assert(service->name);


	switch (GET_STATE(service)) {
	case IS_UP:
		check_syslog();
		initng_log(LOG_NOTICE, NULL, "Service %s is up.\n",
			   service->name);
		break;

	case IS_DOWN:
		initng_log(LOG_NOTICE, NULL, "Service %s has been stopped.\n",
			   service->name);
		break;

	case IS_FAILED:
		initng_log(LOG_NOTICE, NULL, "Service %s FAILED.\n",
			   service->name);
		break;

	case IS_STOPPING:
		initng_log(LOG_NOTICE, NULL, "Service %s is stopping.\n",
			   service->name);
		break;

	case IS_STARTING:
		initng_log(LOG_NOTICE, NULL, "Service %s is starting.\n",
			   service->name);
		break;
	default:
		break;
	}
}

static void syslog_print_system_state(s_event * event)
{
	h_sys_state *state;

	assert(event->event_type == &EVENT_SYSTEM_CHANGE);
	assert(event->data);

	state = event->data;

	switch (*state) {
	case STATE_UP:
		initng_log(LOG_NOTICE, NULL, "System is up and running!\n");
		/*
		 * if syslogd have not been started yet, its no ida,
		 * to spend memory for the buffert anyway.
		 */
		free_buffert();
		break;

	case STATE_STARTING:
		initng_log(LOG_NOTICE, NULL, "System is starting up.\n");
		break;

	case STATE_STOPPING:
		initng_log(LOG_NOTICE, NULL, "System is going down.\n");
		break;

	case STATE_ASE:
		initng_log(LOG_NOTICE, NULL, "Last service exited.\n");
		break;

	case STATE_EXIT:
		initng_log(LOG_NOTICE, NULL, "Initng is exiting.\n");
		break;

	case STATE_RESTART:
		initng_log(LOG_NOTICE, NULL, "Initng is restarting.\n");
		break;

	case STATE_HALT:
		initng_log(LOG_NOTICE, NULL, "System is halting.\n");
		break;

	case STATE_POWEROFF:
		initng_log(LOG_NOTICE, NULL, "System is power-off.\n");
		break;

	case STATE_REBOOT:
		initng_log(LOG_NOTICE, NULL, "System is rebooting.\n");

	default:
		break;
	}
}

static void syslog_fetch_output(s_event * event)
{
	s_event_buffer_watcher_data *data;
	char log[201];
	int pos = 0;
	int i;

	assert(event->event_type == &EVENT_BUFFER_WATCHER);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);

	if (!is(&FORWARD_TO_SYSLOG, data->service))
	    return;

	/* print every line, ending with a '\n' as an own syslog */
	while (data->buffer_pos[pos]) {
		i = 0;
		/* count the number of char before '\n' */
		while (data->buffer_pos[pos + i] &&
		       data->buffer_pos[pos + i] != '\n' && i < 200)
			i++;

		/* copy that many chars to our temporary log array */
		strncpy(log, &data->buffer_pos[pos], i);
		log[i] = '\0';

		/* send it to syslog */
		initng_log(LOG_NOTICE, data->service->name, "%s", log);

		/* step forward */
		pos += i;

		/* and skip the newline if any */
		if (data->buffer_pos[pos])
			pos++;

	}
}

static void syslog_print_error(s_event * event)
{
	s_event_error_message_data *data;
	va_list va;

	assert(event->event_type == &EVENT_ERROR_MESSAGE);
	assert(event->data);

	data = event->data;
	va_copy(va, data->arg);

	assert(data->file);
	assert(data->func);
	assert(data->format);
	char tempspace[200];

	vsnprintf(tempspace, 200, data->format, va);

	switch (data->mt) {
	case MSG_FAIL:
#ifdef DEBUG
		syslog(LOG_EMERG, "\"%s\", %s() #%i FAIL: %s", data->file,
		       data->func, data->line, tempspace);
#else
		syslog(LOG_EMERG, "FAIL: %s", tempspace);
#endif
		break;

	case MSG_WARN:
#ifdef DEBUG
		syslog(LOG_WARNING, "\"%s\", %s() #%i WARN: %s", data->file,
		       data->func, data->line, tempspace);
#else
		syslog(LOG_EMERG, "WARN: %s", tempspace);
#endif
		break;

	default:
		syslog(LOG_NOTICE, "%s", tempspace);
		break;
	}

	va_end(va);
}

int module_init(void)
{
	D_("Initializing syslog module\n");

	initng_service_data_type_register(&FORWARD_TO_SYSLOG);

	initng_list_init(&log_list.list);
	check_syslog();

	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("InitNG", 0, LOG_LOCAL1);

	initng_event_hook_register(&EVENT_IS_CHANGE,
				   &syslog_print_status_change);
	initng_event_hook_register(&EVENT_SYSTEM_CHANGE,
				   &syslog_print_system_state);
	initng_event_hook_register(&EVENT_BUFFER_WATCHER, &syslog_fetch_output);
	initng_event_hook_register(&EVENT_ERROR_MESSAGE, &syslog_print_error);
	initng_event_hook_register(&EVENT_HANDLE_KILLED, &syslog_print_status_killed);

	return TRUE;
}

void module_unload(void)
{
    initng_service_data_type_unregister(&FORWARD_TO_SYSLOG);

    initng_event_hook_unregister(&EVENT_IS_CHANGE,
				     &syslog_print_status_change);
	initng_event_hook_unregister(&EVENT_SYSTEM_CHANGE,
				     &syslog_print_system_state);
	initng_event_hook_unregister(&EVENT_BUFFER_WATCHER,
				     &syslog_fetch_output);
	initng_event_hook_unregister(&EVENT_ERROR_MESSAGE, &syslog_print_error);
	initng_event_hook_unregister(&EVENT_HANDLE_KILLED, &syslog_print_status_killed);
	free_buffert();
	closelog();
}
