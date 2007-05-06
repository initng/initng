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

#include <sys/time.h>
#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <sys/un.h>							/* memmove() strcmp() */
#include <sys/wait.h>						/* waitpid() sa */
#include <linux/kd.h>						/* KDSIGACCEPT */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdio.h>							/* printf() */
#include <stdlib.h>							/* free() exit() */
#include <sys/reboot.h>						/* reboot() RB_DISABLE_CAD */
#include <assert.h>

#include "local.h"


/*
 * when a service fails to start, this function
 * walks through all its dependencies, and mark it "dependency failed to start"
 */
void dep_failed_to_start(active_db_h * service)
{
	/* TODO, find a way to handle this */
#ifdef NONO
	active_db_h *current = NULL;

	/* walk over all services */
	while_active_db(current)
	{
		if (current == service)
			continue;

		/* if current depends on service failed to start */
		if (initng_depend(current, service) == TRUE)
			initng_common_mark_service(current, &START_DEP_FAILED);
	}
#endif
}

#ifdef DEP_FAILED_TO_STOP
/*
 * when a service fails to stop, this function
 * walks through all its dependencies, and mark it "dependency failed to stop"
 */
void dep_failed_to_stop(active_db_h * service)
{
	active_db_h *current = NULL;

	/* walk over all services */
	while_active_db(current)
	{
		if (current == service)
			continue;

		/* if current depends on service failed to stop */
		if (initng_active_db_dep_on(current, service) == TRUE)
			initng_common_mark_service(current, STOP_DEP_FAILED);
	}
}
#endif
