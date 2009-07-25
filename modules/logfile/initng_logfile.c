/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2005 neuron <aagaande@gmail.com>
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
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};


s_entry LOGFILE = {
	.name = "logfile",
	.description = "An extra output of service output.",
	.type = STRING,
	.ot = NULL,
};

static void program_output(s_event * event)
{
	s_event_buffer_watcher_data *data;
	const char *filename = NULL;
	int len = 0;
	int fd = -1;

	assert(event->event_type == &EVENT_BUFFER_WATCHER);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);
	assert(data->process);

	/*D_("%s process fd: # %i, %i, service %s, have something to say\n",
	   data->process->pt->name, data->process->out_pipe[0],
	   data->process->out_pipe[1], data->service->name); */

	/* get the filename */
	filename = get_string(&LOGFILE, data->service);
	if (!filename) {
		D_("Logfile not set\n");
		return;
	}

	/* open the file */
	if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 1) {
		F_("Error opening %s, err : %s\n", filename, strerror(errno));
		return;
	}

	/* Write data to logfile */
	D_("Writing data...\n");
	len = strlen(data->buffer_pos);

	if (write(fd, data->buffer_pos, len) != len)
		F_("Error writing to %s's log, err : %s\n",
		   data->service->name, strerror(errno));

	close(fd);
}

int module_init(void)
{
	initng_service_data_type_register(&LOGFILE);

	initng_event_hook_register(&EVENT_BUFFER_WATCHER, &program_output);
	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&LOGFILE);
	initng_event_hook_unregister(&EVENT_BUFFER_WATCHER, &program_output);
}
