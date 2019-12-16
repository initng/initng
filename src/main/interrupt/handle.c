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

#include "local.h"

void handle(active_db_h * service)
{
	s_event event;
	a_state_h *state = service->current_state;

	event.event_type = &EVENT_STATE_CHANGE;
	event.data = service;

	/* lock the state, so we could know about a change */
	initng_common_state_lock(service);

	/* call all hooks again, to notify about the change */
	initng_event_send(&event);

	/* abort if the state has changed since the event was sent */
	if (initng_common_state_unlock(service))
		return;

	/* If the rough state has changed */
	if (service->last_rought_state != state->is) {
		D_("An is change from %i to %i for %s.\n",
		   service->last_rought_state, state->is, service->name);

		initng_common_state_lock(service);

		event.event_type = &EVENT_IS_CHANGE;
		initng_event_send(&event);

		/* abort if the state has changed since the event was sent */
		if (initng_common_state_unlock(service))
			return;

		switch (GET_STATE(service)) {
		/* this will make all services, that depend of this,
		 * DEP_FAILED_TO_START */
		case IS_FAILED:
			dep_failed_to_start(service);
			/* fall thru */

		/* This checks if all services on a runlevel is up, then set
		 * STATE_UP */
		case IS_UP:
			check_sys_state_up();
			break;

		/* if this service is marked restarting, please restart it if
		 * its set to STOPPED */
		case IS_DOWN:
			if (is(&RESTARTING, service))
				initng_handler_restart_restarting();

		default:
			break;
		}
		/* this will make all services, that depend of this to stop,
		 * DEP_FAILED_TO_STOP */
		/* TODO
		   if (IS_MARK(service, &FAIL_STOPPING))
		   {
		   #ifdef DEP_FAIL_TO_STOP
		   dep_failed_to_stop(service);
		   #endif
		   check_sys_state_up();
		   }
		 */
	}

	/* Run state init hook if present */
	if (service->current_state->init)
		(*service->current_state->init) (service);

	D_("service %s is now %s.\n", service->name,
	   service->current_state->name);
}
