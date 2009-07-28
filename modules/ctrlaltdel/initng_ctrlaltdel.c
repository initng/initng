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
#include <signal.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};


static void ctrlaltdel(s_event * event)
{
	int *signal;

	assert(event->event_type == &EVENT_SIGNAL);

	signal = event->data;

	if (*signal != SIGINT)
		return;

	/* what to do when there is no services left */
	g.when_out = THEN_REBOOT;

	/* stop all services */
	initng_handler_stop_all();
}

int module_init(void)
{
	initng_event_hook_register(&EVENT_SIGNAL, &ctrlaltdel);
	return TRUE;
}

void module_unload(void)
{
	initng_event_hook_unregister(&EVENT_SIGNAL, &ctrlaltdel);
}
