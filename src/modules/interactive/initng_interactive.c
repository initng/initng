/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2005 Jimmy Wennlund <jimmy.wennlund@gmail.com>
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
#include <string.h>		/* strstr() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};


static int active;

static void interactive_STARTING(s_event * event);
static void interactive_STOP_MARKED(s_event * event);

a_state_h INT_DISABLED = {
	.name = "INTERACTIVELY_DISABLED",
	.description = "The user choose to not start this service.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

static void interactive_STARTING(s_event * event)
{
	active_db_h *service;
	char asw[10];

	assert(event->event_type == &EVENT_START_DEP_MET);
	assert(event->data);

	service = event->data;

	asw[0] = '\0';

	fprintf(stderr, "Start service %s, (Y/n/a):", service->name);
	/* HACK: ignore read errors by assuming 'n' */
	if (!fgets(asw, 9, stdin))
		asw[0] = 'n';

	/* if it is true, then its ok to launch service */
	if (asw[0] == 'y' || asw[0] == 'Y')
		return;

	if (asw[0] == 'a' || asw[0] == 'A') {
		initng_event_hook_unregister(&EVENT_START_DEP_MET,
					     &interactive_STARTING);
		initng_event_hook_unregister(&EVENT_STOP_DEP_MET,
					     &interactive_STOP_MARKED);
		active = FALSE;
		return;
	}

	initng_common_mark_service(service, &INT_DISABLED);

	event->status = FAILED;
}

static void interactive_STOP_MARKED(s_event * event)
{
	active_db_h *service;
	char asw[10];

	assert(event->event_type == &EVENT_STOP_DEP_MET);
	assert(event->data);

	service = event->data;

	asw[0] = '\0';

	fprintf(stderr, "Stop service %s, (Y/n/a):", service->name);
	/* HACK: ignore read errors by assuming 'n' */
	if (!fgets(asw, 9, stdin))
		asw[0] = 'n';

	if (asw[0] == 'y' || asw[0] == 'Y')
		return;

	if (asw[0] == 'a' || asw[0] == 'A') {
		initng_event_hook_unregister(&EVENT_START_DEP_MET,
					     &interactive_STARTING);
		initng_event_hook_unregister(&EVENT_STOP_DEP_MET,
					     &interactive_STOP_MARKED);
		active = FALSE;
		return;
	}

	initng_common_mark_service(service, &INT_DISABLED);

	event->status = FAILED;
}

int module_init(void)
{
	int i;

	/* look for the string interactive */
	for (i = 0; g.Argv[i]; i++) {
		if (strstr(g.Argv[i], "interactive")) {	/* if found */
			P_("Initng is started in interactive mode!\n");
			initng_event_hook_register(&EVENT_START_DEP_MET,
						   &interactive_STARTING);
			initng_event_hook_register(&EVENT_STOP_DEP_MET,
						   &interactive_STOP_MARKED);
			active = TRUE;
			return TRUE;
		}
	}

	active = FALSE;
	initng_module_unload_named("interactive");
	return TRUE;
}

void module_unload(void)
{
	if (active == TRUE) {
		initng_event_hook_unregister(&EVENT_START_DEP_MET,
					     &interactive_STARTING);
		initng_event_hook_unregister(&EVENT_STOP_DEP_MET,
					     &interactive_STOP_MARKED);
	}
}
