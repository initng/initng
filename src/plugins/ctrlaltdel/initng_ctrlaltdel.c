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

#include <stdio.h>
#include <stdlib.h>					/* free() exit() */
#include <string.h>
#include <assert.h>
#include <signal.h>


INITNG_PLUGIN_MACRO;

static void ctrlaltdel(s_event * event)
{
	int *signal;

	assert(event->event_type == &EVENT_SIGNAL);

	signal = event->data;

	if (*signal != SIGINT)
		return;

	/* what to do when there is no services left */
	if (g.i_am == I_AM_INIT)
		g.when_out = THEN_REBOOT;
	else if (g.i_am == I_AM_FAKE_INIT)
		g.when_out = THEN_QUIT;

	/* stop all services */
	initng_handler_stop_all();
}

int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_event_hook_register(&EVENT_SIGNAL, &ctrlaltdel);
	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_event_hook_unregister(&EVENT_SIGNAL, &ctrlaltdel);
}
