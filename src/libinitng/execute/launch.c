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

#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <unistd.h>							/* execv() usleep() pause() chown() */
#include <sys/wait.h>						/* waitpid() sa */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdlib.h>							/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>


int initng_execute_launch(active_db_h * service, ptype_h * type,
			  const char *exec_name)
{
	process_h *process = NULL;
	int ret = 0;

	assert(service);
	assert(type);

	D_("start_launch(%s, %s);\n", service->name, type->name);

	/* Try to get the current one */
	process = initng_process_db_get(type, service);
	if (process != NULL)
	{
		F_("There exists an \"%s\" process in \"%s\" already.\n",
		   process->pt->name, service->name);
		/*assert(NULL); */
		return (FALSE);
	}

	/* Try to create a new one */
	process = initng_process_db_new(type);
	if (!process)
	{
		F_("(%s): unable to allocate start_process!\n", service->name);
		return (FALSE);
	}

	/* add the process to our service */
	list_add(&process->list, &service->processes.list);

	/* make sure we have a exec_name */
	if (!exec_name)
		exec_name = type->name;

	/* send the event */
	{
		s_event event;
		s_event_launch_data data;

		event.event_type = &EVENT_LAUNCH;
		event.data = &data;

		data.service = service;
		data.process = process;
		data.exec_name = exec_name;

		initng_event_send(&event);
		ret = event.status;
	}

	/* if we found a launcher */
	if (ret == HANDLED)
		return (TRUE);

	if (ret == FAILED)
		F_("initng_execute(%s): FAILED LAUNCHING, returned FAIL\n",
		   service->name);
	else
		D_("initng_execute(%s): FAILED LAUNCHING, noting found to launch.\n",
		   service->name);

	/* on failure remove the process from list, and free it */
	initng_process_db_free(process);

	return (ret);
}
