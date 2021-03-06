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
#include <string.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

s_entry S_DELAY = {
	.name = "exec_delay",
	.description = "Pause this much seconds before service is launched.",
	.type = VARIABLE_INT,
	.ot = NULL,
};

s_entry MS_DELAY = {
	.name = "exec_m_delay",
	.description = "Pause this much microseconds before service is "
	    "launched.",
	.type = VARIABLE_INT,
	.ot = NULL,
};

static void do_pause(s_event * event)
{
	s_event_after_fork_data *data;

	int s_delay = 0;
	int ms_delay = 0;

	assert(event->event_type == &EVENT_AFTER_FORK);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->process);
	assert(data->process->pt);

	D_(" %s\n", data->service->name);

	s_delay = get_int_var(&S_DELAY, data->process->pt->name, data->service);
	ms_delay = get_int_var(&MS_DELAY, data->process->pt->name,
			       data->service);

	if (s_delay) {
		D_("Sleeping for %i seconds.\n", s_delay);
		sleep(s_delay);
	}

	if (ms_delay) {
		D_("Sleeping for %i milliseconds.\n", ms_delay);
		usleep(ms_delay);
	}
}

int module_init(void)
{
	initng_service_data_type_register(&S_DELAY);
	initng_service_data_type_register(&MS_DELAY);
	return (initng_event_hook_register(&EVENT_AFTER_FORK, &do_pause));
}

void module_unload(void)
{
	initng_service_data_type_unregister(&S_DELAY);
	initng_service_data_type_unregister(&MS_DELAY);
	initng_event_hook_unregister(&EVENT_AFTER_FORK, &do_pause);
}
