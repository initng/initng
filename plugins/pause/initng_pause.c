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
#include <string.h>
#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <assert.h>

INITNG_PLUGIN_MACRO;

s_entry S_DELAY = { "exec_delay", VARIABLE_INT, NULL,
	"Pause this much seconds before service is launched."
};

s_entry MS_DELAY = { "exec_m_delay", VARIABLE_INT, NULL,
	"Pause this much microseconds before service is launched."
};

static int do_pause(active_db_h * s, process_h * p)
{
	int s_delay = 0;
	int ms_delay = 0;

	assert(s);
	assert(p);
	assert(p->pt);

	D_(" %s\n", s->name);


	s_delay = get_int_var(&S_DELAY, p->pt->name, s);
	ms_delay = get_int_var(&MS_DELAY, p->pt->name, s);


	if (s_delay)
	{
		D_("Sleeping for %i seconds.\n", s_delay);
		sleep(s_delay);
	}

	if (ms_delay)
	{
		D_("Sleeping for %i milliseconds.\n", ms_delay);
		usleep(ms_delay);
	}

	return (TRUE);
}

int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_service_data_type_register(&S_DELAY);
	initng_service_data_type_register(&MS_DELAY);
	return (initng_plugin_hook_register(&g.A_FORK, 90, &do_pause));
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_service_data_type_unregister(&S_DELAY);
	initng_service_data_type_unregister(&MS_DELAY);
	initng_plugin_hook_unregister(&g.A_FORK, &do_pause);
}
