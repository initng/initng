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

#include <stdio.h>
#include <stdlib.h>		/* free() exit() */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <libgen.h>
#include <sys/inotify.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

/*
 * ############################################################################
 * #                            CONSTANT DEFINES                              #
 * ############################################################################
 */

/*
 * maximum of fds
 */
#define MAX_WATCH                    1024

/*
 * inotify event buffer
 */
#define BUF_LEN                      (MAX_WATCH * (sizeof(struct inotify_event) + 16))

/*
 * ############################################################################
 * #                            FD HANDLER                                    #
 * ############################################################################
 */

static void filemon_event(f_module_h * from, e_fdw what);
static void fdh_handler(s_event * event);
static void add_watch(const char *path, active_db_h *service);
static void remove_watch(active_db_h *service);

/* this module file descriptor we add to monitor */
f_module_h fdh = {
	.call_module = &filemon_event,
	.what = IOW_READ,
	.fds = -1
};

/* saved to be cleaned later on */
typedef struct {
	/* the name of the service */
	char *name;

	/* the list this struct is in */
	list_t list;
} a_service_name;

a_service_name* watch_to_service_name[MAX_WATCH];

/* generic fd handler */
void fdh_handler(s_event * event)
{
	s_event_io_watcher_data *data;

	assert(event);
	assert(event->data);

	data = event->data;

	switch (data->action) {
	case IOW_ACTION_CLOSE:
		if (fdh.fds > 0)
			close(fdh.fds);
		break;

	case IOW_ACTION_CHECK:
		/* skip sdout(0), stdin(1), stderr(2) */
		if (fdh.fds <= 2)
			break;

		/* This is a expensive test, but better safe then sorry */
		if (!STILL_OPEN(fdh.fds)) {
			D_("%i is not open anymore.\n", fdh.fds);
			fdh.fds = -1;
			break;
		}

		FD_SET(fdh.fds, data->readset);
		data->added++;
		break;

	case IOW_ACTION_CALL:
		/* skip sdout(0), stdin(1), stderr(2) */
		if (!data->added || fdh.fds <= 2)
			break;

		if (!FD_ISSET(fdh.fds, data->readset))
			break;

		filemon_event(&fdh, IOW_READ);
		data->added--;
		break;

	case IOW_ACTION_DEBUG:
		if (!data->debug_find_what ||
		    strstr(__FILE__, data->debug_find_what)) {
			initng_string_mprintf(data->debug_out, " %i: Used by module: %s\n",
				fdh.fds, __FILE__);
		}
		break;
	}
}

/* add watcher */
void add_watch(const char *path, active_db_h *service) {
	D_("add_watch for '%s' -> '%s'\n", path, service->name);
	/* get first existing folder and monitor it*/
	struct stat file_stat;
	char *path_copy = initng_toolbox_strdup(path);
	char *dname = path_copy;
	do
	{
		dname = dirname(dname);
		D_("trying '%s' ...\n", dname);
	}
	while (stat(dname, &file_stat) != 0);
	int watch = inotify_add_watch(fdh.fds, dname, IN_CREATE);
	if (watch < 0) {
		F_("fail to monitor \"%s\"\n", dname);
		return;
	}
	D_("watch '%d' on path '%s' ...\n", watch, dname);
	free(path_copy);
	/* update watch to service name list */
	/* service name list is null */
	a_service_name *s_name = watch_to_service_name[watch];
	if (s_name == NULL) {
		D_("create watch list '%d' on path!\n", watch);
		/* init list */
		s_name = initng_toolbox_calloc(1, sizeof(a_service_name));
		initng_list_init(&s_name->list);
		watch_to_service_name[watch] = s_name;
		/* add first element */
		a_service_name *current = initng_toolbox_calloc(1, sizeof(a_service_name));
		current->name = initng_toolbox_strdup(service->name);
		initng_list_add(&current->list, &s_name->list);
		D_("+ new watch '%d' for service '%s'!\n", watch, current->name );
	}
	/* service name list exists */
	else {
		D_("found existing watch list '%d' on path!\n", watch);
		/* verify service not already in the list */
		D_("checking registered services...\n");
		a_service_name *current = NULL;
		initng_list_foreach(current, &s_name->list, list) {
			D_("+ service '%s'\n", current->name);
			if (strcmp(current->name, service->name) == 0) {
				/* service already in the list */
				D_("+ watch already set for service '%s'!\n", service->name);
				return;
			}
		}
		/* service name was not found in the list, add it */
		current = initng_toolbox_calloc(1, sizeof(a_service_name));
		current->name = initng_toolbox_strdup(service->name);
		initng_list_add(&current->list, &s_name->list);
		D_("+ adding watch '%d' for service '%s'\n", watch, current->name);
	}
}

void remove_watch(active_db_h *service) {
	D_("removing watch for service '%s' ...\n", service->name);
	for (int watch = 0; watch < MAX_WATCH; ++watch ) {
		a_service_name *s_name = watch_to_service_name[watch];
		if (s_name == NULL) {
			continue;
		}
		a_service_name *current = NULL;
		a_service_name *safe = NULL;
		initng_list_foreach_safe(current, safe, &s_name->list, list) {
			if (strcmp(current->name, service->name) == 0) {
				initng_list_del(&current->list);
				free(current->name);
				free(current);
				D_("+ '%d' removed service '%s'\n", watch, service->name);
			}
		}
		if (initng_list_isempty(&s_name->list)) {
			inotify_rm_watch(fdh.fds, watch);
			free(s_name);
			watch_to_service_name[watch] = NULL;
			D_("+ '%d' removed empty watch\n", watch);
		}
	}
}

/* called by fd hook, when there is data */
void filemon_event(f_module_h * from, e_fdw what)
{
	int len = 0;
	int i = 0;

	char buf[BUF_LEN];

	/* read events */
	len = read(from->fds, buf, BUF_LEN);

	/* if error */
	if (len < 0) {
		F_("fstat read error\n");
		return;
	}

	/* handle all */
	while (i < len) {
		struct inotify_event *event;

		event = (struct inotify_event *)&buf[i];

		if (event->mask & IN_CREATE) {
			a_service_name *s_name = watch_to_service_name[event->wd];
			if (s_name != NULL && event->len) {
				a_service_name *current = NULL;
				a_service_name *safe = NULL;
				initng_list_foreach_safe(current, safe, &s_name->list, list) {
					active_db_h *service =  initng_active_db_find_by_name(current->name);
					if (service == NULL) {
						D_("service \"%s\" not found, removing\n", current->name);
						initng_list_del(&current->list);
						free(current->name);
						free(current);
					}
					else if (service->current_state->interrupt) {
						D_("triggering '%d' on '%s'...\n", event->wd, service->name);
						(*service->current_state->interrupt) (service);
					}
				}
				return;
			}
		}

		i += sizeof(struct inotify_event) + event->len;
	}
}

/*
 * ############################################################################
 * #                       FSTAT VARIABLES                                   #
 * ############################################################################
 */

s_entry WAIT_FOR_FILE = {
	.name = "wait_for_file",
	.description = "Check so that this files exits before launching.",
	.type = STRINGS,
	.ot = NULL,
};

s_entry REQUIRE_FILE = {
	.name = "require_file",
	.description = "If this file do not exist, this service will FAIL "
	    "directly.",
	.type = STRINGS,
	.ot = NULL,
};

s_entry WAIT_FOR_FILE_AFTER = {
	.name = "wait_for_file_after",
	.description = "Make sure this files exits before a service can be "
	    "marked as up.",
	.type = STRINGS,
	.ot = NULL,
};

s_entry REQUIRE_FILE_AFTER = {
	.name = "require_file_after",
	.description = "If this file do not exist after, the service will be "
	    "marked FAIL.",
	.type = STRINGS,
	.ot = NULL,
};

/*
 * ############################################################################
 * #                       FSTAT STATE  STRUCTS                               #
 * ############################################################################
 */

a_state_h REQUIRE_FILE_FAILED = {
	.name = "REQUIRE_FILE_FAILED",
	.description = "A file that is required to exist before this service "
	    "can start was not found.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h REQUIRE_FILE_AFTER_FAILED = {
	.name = "REQUIRE_FILE_AFTER_FAILED",
	.description = "A file that is required to exist after this service "
	    "is started was not found.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTIONS                         #
 * ############################################################################
 */

/*
 * Do this check before START_DEP_MET can be set.
 */
static void check_files_to_exist(s_event * event)
{
	active_db_h *service;
	const char *file = NULL;
	s_data *itt = NULL;
	struct stat file_stat;

	assert(event->event_type == &EVENT_START_DEP_MET);
	assert(event->data);

	service = event->data;

	/* CHECK WAIT_FOR_FILE */
	while ((file = get_next_string(&WAIT_FOR_FILE, service, &itt))) {
		D_("Service %s waits for file %s to exist\n", service->name, file);
		if (stat(file, &file_stat) != 0) {
			/* don't change status of service */
			event->status = FAILED;
			add_watch(file, service);
			return;
		}
	}

	/* CHECK REQUIRE_FILE */
	while ((file = get_next_string(&REQUIRE_FILE, service, &itt))) {
		D_("Service %s requires file %s to exist\n", service->name, file);
		if (stat(file, &file_stat) != 0) {
			initng_common_mark_service(service,
						   &REQUIRE_FILE_FAILED);
			event->status = FAILED;
			return;
		}
	}

	remove_watch(service);
}

/*
 * Do this test before status RUNNING can be set.
 */
static void check_files_to_exist_after(s_event * event)
{
	active_db_h *service;
	const char *file = NULL;
	s_data *itt = NULL;

	struct stat file_stat;

	assert(event->event_type == &EVENT_UP_MET);
	assert(event->data);

	service = event->data;

	/* check WAIT_FOR_FILE_AFTER */
	while ((file = get_next_string(&WAIT_FOR_FILE_AFTER, service, &itt))) {
		D_("Service %s waits for file %s before it can be set to "
		   "RUNNING\n", service->name, file);
		if (stat(file, &file_stat) != 0) {
			/* don't change status of service */
			event->status = FAILED;
			add_watch(file, service);
			return;
		}
	}

	/* check REQUIRE_FILE_AFTER */
	while ((file = get_next_string(&WAIT_FOR_FILE_AFTER, service, &itt))) {
		if (stat(file, &file_stat) != 0) {
			D_("Service %s requires file %s before it can be set to "
			   "RUNNING\n", service->name, file);
			initng_common_mark_service(service,
						   &REQUIRE_FILE_AFTER_FAILED);
			event->status = FAILED;
			return;
		}
	}

	remove_watch(service);
}

int module_init(void)
{
	/* zero globals */
	fdh.fds = inotify_init();

	/* add this hook */
	initng_event_hook_register(&EVENT_IO_WATCHER, &fdh_handler);

	initng_service_data_type_register(&WAIT_FOR_FILE);
	initng_service_data_type_register(&REQUIRE_FILE);
	initng_service_data_type_register(&WAIT_FOR_FILE_AFTER);
	initng_service_data_type_register(&REQUIRE_FILE_AFTER);

	initng_active_state_register(&REQUIRE_FILE_FAILED);
	initng_active_state_register(&REQUIRE_FILE_AFTER_FAILED);

	initng_event_hook_register(&EVENT_START_DEP_MET, &check_files_to_exist);
	initng_event_hook_register(&EVENT_UP_MET, &check_files_to_exist_after);

	return TRUE;
}

void module_unload(void)
{
	/* remove all watchers */
	for (int watch = 0; watch < MAX_WATCH; ++watch ) {
		a_service_name *s_name = watch_to_service_name[watch];
		if (s_name == NULL) {
			continue;
		}
		inotify_rm_watch(fdh.fds, watch);
		a_service_name *current = NULL;
		a_service_name *safe = NULL;
		initng_list_foreach_safe(current, safe, &s_name->list, list) {
			initng_list_del(&current->list);
			free(current->name);
			free(current);
		}
		free(s_name);
		watch_to_service_name[watch] = NULL;
	}

	/* close sockets */
	close(fdh.fds);

	initng_service_data_type_unregister(&WAIT_FOR_FILE);
	initng_service_data_type_unregister(&REQUIRE_FILE);
	initng_service_data_type_unregister(&WAIT_FOR_FILE_AFTER);
	initng_service_data_type_unregister(&REQUIRE_FILE_AFTER);

	initng_active_state_unregister(&REQUIRE_FILE_FAILED);
	initng_active_state_unregister(&REQUIRE_FILE_AFTER_FAILED);

	initng_event_hook_unregister(&EVENT_START_DEP_MET,
				     &check_files_to_exist);
	initng_event_hook_unregister(&EVENT_UP_MET,
				     &check_files_to_exist_after);
}
