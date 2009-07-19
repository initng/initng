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
#include <string.h>		/* strstr() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

static int module_init(void);
static int module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};


s_entry CONFLICT = {
	.name = "conflict",
	.description = "If service put here is starting or running, bail "
	    "out.",
	.type = STRINGS,
	.ot = NULL,
};

a_state_h CONFLICTING = {
	.name = "FAILED_BY_CONFLICT",
	.description = "There is a running service that is conflicting with "
	    "this service, initng cannot launch this service.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

static void check_conflict(s_event * event)
{
	active_db_h *service;
	const char *conflict_entry = NULL;
	s_data *itt = NULL;

	assert(event->event_type == &EVENT_IS_CHANGE);
	assert(event->data);

	service = event->data;

	assert(service->name);

	/* Do this check when this service is put in a STARTING state */
	if (!IS_STARTING(service))
		return;

	/* make sure the conflict entry is set */
	while ((conflict_entry = get_next_string(&CONFLICT, service, &itt))) {
		active_db_h *s = NULL;

		/*D_("Making sure that %s is not running.\n", conflict_entry); */

		s = initng_active_db_find_by_name(conflict_entry);

		/* this is actually good */
		if (!s)
			continue;

		if (IS_UP(s) || IS_STARTING(s)) {
			initng_common_mark_service(service, &CONFLICTING);
			F_("Service \"%s\" is conflicting with service "
			   "\"%s\"!\n", service->name, s->name);
			return;
		}
	}
}

int module_init(void)
{
	initng_service_data_type_register(&CONFLICT);
	initng_event_hook_register(&EVENT_IS_CHANGE, &check_conflict);
	initng_active_state_register(&CONFLICTING);

	return TRUE;
}

void module_unload(void)
{
	initng_event_hook_unregister(&EVENT_IS_CHANGE, &check_conflict);
	initng_active_state_unregister(&CONFLICTING);
	initng_service_data_type_unregister(&CONFLICT);
}
