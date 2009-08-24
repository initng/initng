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

#include <sys/time.h>
#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>
#include <errno.h>

/* Load new process named */
active_db_h *initng_handler_start_new_service_named(const char *service)
{
	active_db_h *to_load = NULL;

	D_(" start_new_service_named(%s);\n", service);

	assert(service);

	/* Try to find it */
	if ((to_load = initng_active_db_find_by_name(service))) {
		if (!IS_DOWN(to_load)) {
			D_("Service %s exits already, and is not stopped!\n",
			   to_load->name);
			return to_load;
		}

		/* okay, now start it */
		initng_handler_start_service(to_load);

		return to_load;
	}

	/* get from hook */
	if ((to_load = initng_plugin_active_new(service)))
		return to_load;

	/* the function calling this function will print out an error */
	D_("Unable to load active for service %s\n", service);
	return NULL;
}
