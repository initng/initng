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

int initng_handler_restart_service(active_db_h * service_to_restart)
{
	S_;
	assert(service_to_restart);
	assert(service_to_restart->name);
	assert(service_to_restart->current_state);

	/* type has to be known to restart the service */
	if (!service_to_restart->type)
		return FALSE;

	if (!IS_UP(service_to_restart)) {
		D_("Can only restart a running service %s. "
		   "(now_state : %s)\n", service_to_restart->name,
		   service_to_restart->current_state->name);
		return FALSE;
	}

	/* Restart all dependencys to this service */
	initng_depend_restart_deps(service_to_restart);

	/* if there exits a restart code, use it */
	if (service_to_restart->type->restart)
		return (*service_to_restart->type->
			restart) (service_to_restart);

	/* else, mark the service for restarting and stop it */
	set(&RESTARTING, service_to_restart);
	return initng_handler_stop_service(service_to_restart);
}
