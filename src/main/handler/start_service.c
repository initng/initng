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

/* When this function is called, the service is marked for starting */
int initng_handler_start_service(active_db_h * service_to_start)
{
	assert(service_to_start);
	assert(service_to_start->name);
	assert(service_to_start->current_state);

	D_("start_service(%s);\n", service_to_start->name);

	if (!service_to_start->type) {
		F_("Type for service %s not set.\n", service_to_start->name);
		return FALSE;
	}

	/* check system state, if we can launch. */
	if (g.sys_state != STATE_STARTING && g.sys_state != STATE_UP) {
		F_("Can't start service %s, when system status is: %i !\n",
		   service_to_start->name, g.sys_state);
		return FALSE;
	}

	/* else, mark the service for restarting and stop it */
	if (is(&RESTARTING, service_to_start)) {
		D_("Cant manually start a service that is restarting.\n");
		return FALSE;
	}


	switch (GET_STATE(service_to_start)) {
	/* it might already be starting */
	case IS_STARTING:
        case IS_WAITING:
		D_("service %s is starting already.\n", service_to_start->name);
		return TRUE;

	/* if it failed */
	case  IS_FAILED:
		D_("Service %s failed and must be reset before it can be "
		   "started again!\n", service_to_start->name);
		return FALSE;

	/* it might already be up */
	case IS_UP:
		D_("service %s is already up!\n", service_to_start->name);
		return TRUE;

	/* if new, and not got a stopped state yet, its no idea to bug this
	 * process */
	case IS_NEW:
		D_("service %s is so fresh so we cant start it.\n",
		   service_to_start->name);
		return TRUE;

	/* it must be down or stopping to start it */
	case IS_STOPPING:
	case IS_DOWN:
		break;

	default:
		W_("Can't set a service with status %s, to start\n",
		   service_to_start->current_state->name);
		return FALSE;
	}

	/* This will run this functuin (start_service) for all dependecys
	 * this service have. */
	if (initng_depend_start_deps(service_to_start) != TRUE) {
		D_("Could not start %s, because a required dependency could "
		   "not be found.\n", service_to_start->name);
		return FALSE;
	}

	/* Now execute the local start_service code, in the service type
	 * module. */
	if (!service_to_start->type->start)
		return FALSE;

	/* execute it */
	return ((*service_to_start->type->start) (service_to_start));
}
