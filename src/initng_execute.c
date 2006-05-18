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

#include "initng.h"

#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <unistd.h>							/* execv() pipe() usleep() pause() chown() */
#include <sys/wait.h>						/* waitpid() sa */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdlib.h>							/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>

#include "initng_global.h"
#include "initng_process_db.h"
#include "initng_string_tools.h"			/* split() */
#include "initng_execute.h"
#include "initng_active_db.h"
#include "initng_plugin_callers.h"
#include "initng_toolbox.h"
#include "initng_common.h"
#include "initng_static_data_id.h"
#include "initng_plugin.h"

int initng_execute_launch(active_db_h * service, ptype_h * type,
						  const char *exec_name)
{
	s_call *current, *q = NULL;
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

	/* walk the db with hooks */
	while_list_safe(current, &g.LAUNCH, q)
	{
		/* call the start */
		ret = (*current->c.launch) (service, process, exec_name);

		/* if launch succeeded */
		if (ret >= TRUE)
			return (TRUE);

		/* if that launcher did not get any to launch, take next launcher */
		if (ret == FALSE)
			continue;
		if (ret < FALSE)
			break;
	}

	if (ret < FALSE)						/* ret == FAIL */
		F_("initng_execute(%s): FAILED LAUNCHING, returned FAIL\n",
		   service->name);
	else
		D_("initng_execute(%s): FAILED LAUNCHING, noting font to launch.\n",
		   service->name);

	/* on failure remove the process from list, and free it */
	initng_process_db_free(process);

	return (ret);
}
