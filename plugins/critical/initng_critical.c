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
#include <assert.h>

#include "active_db_print.h"

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};

s_entry CRITICAL = {
	.name = "critical",
	.description = "If this option is set, and service doesn't succeed, "
	    "initng will quit and offer a sulogin.",
	.type = SET,
	.ot = NULL,
};

/* returns TRUE if all use deps are started */
static void check_critical(s_event * event)
{
	active_db_h *service;

	assert(event->event_type == &EVENT_IS_CHANGE);
	assert(event->data);

	service = event->data;

	assert(service->name);

	if (!IS_FAILED(service))
		return;

	if (!is(&CRITICAL, service))
		return;

	F_("Service %s failed, this is critical, going su_login!!\n",
	   service->name);

	// Sleep for 4 seconds make moast of running procces to end.
	sleep(4);

	active_db_print(service);

	initng_main_su_login();
	/* now try to reload service from disk and run it again */

	/* Reset the service state */
	initng_common_mark_service(service, &NEW);

	/* start the service again */
	initng_handler_start_service(service);

	/* Make sure full runlevel starting fine */
	if (!initng_active_db_find_by_name(g.runlevel)) {
		if (!initng_handler_start_new_service_named(g.runlevel)) {
			F_("runlevel \"%s\" could not be executed!\n",
			   g.runlevel);
		}
	}

	return;
}

int module_init(void)
{
	initng_service_data_type_register(&CRITICAL);
	initng_event_hook_register(&EVENT_IS_CHANGE, &check_critical);
	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&CRITICAL);
	initng_event_hook_unregister(&EVENT_IS_CHANGE, &check_critical);
}
