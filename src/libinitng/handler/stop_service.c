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
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <assert.h>
#include <errno.h>

int initng_handler_stop_service(active_db_h * service_to_stop)
{

	assert(service_to_stop);
	assert(service_to_stop->name);
	assert(service_to_stop->current_state);

	D_("stop_service(%s);\n", service_to_stop->name);

	if (!service_to_stop->type) {
		F_("Service %s type is missing!\n", service_to_stop->name);
		return FALSE;
	}

	/* check so it not failed */
	if (IS_FAILED(service_to_stop)) {
		D_("Service %s is set filed, and cant be stopped.\n",
		   service_to_stop->name);
		return TRUE;
	}

	/* IF service is stopping, do nothing. */
	if (IS_STOPPING(service_to_stop)) {
		D_("service %s is stopping already!\n", service_to_stop->name);
		return TRUE;
	}

	/* check if its currently already down */
	if (IS_DOWN(service_to_stop)) {
		D_("Service %s is down already.\n", service_to_stop->name);
		return TRUE;
	}

	/* must be up or starting, to stop */
	if (!(IS_UP(service_to_stop) || IS_STARTING(service_to_stop))) {
		W_("Service %s is not up but %s, and cant be stopped.\n",
		   service_to_stop->name, service_to_stop->current_state->name);

		return FALSE;
	}

	/* if stop_service code is included in type, use it. */
	if (!service_to_stop->type->stop) {
		W_("Service %s Type %s  has no stopper, will return FALSE!\n",
		   service_to_stop->name, service_to_stop->type->name);
		return FALSE;
	}

	return ((*service_to_stop->type->stop) (service_to_stop));
}
