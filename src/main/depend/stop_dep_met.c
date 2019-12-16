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
#include <stdlib.h>		/* free() exit() */
#include <string.h>
#include <assert.h>

/*
 * This will check with plug-ins if dependencies for stop is met.
 * If this returns FALSE deps for stopping are not met, try again later.
 * If this returns FAIL stop deps, wont EVER be met, stop trying.
 */
int initng_depend_stop_dep_met(active_db_h * service, int verbose)
{
	active_db_h *currentA = NULL;
	int count = 0;

	assert(service);

	/*
	 * Check so all deps, that needs service, is down.
	 * if there are services depending on this one still running,
	 * return false and still try.
	 */
	while_active_db(currentA) {
		count++;
		if (service->depend_cache >= count)
			continue;

		/* temporary increase depend_cache - will degrese before
		 * reutrn (FALSE) */
		service->depend_cache++;

		if (currentA == service)
			continue;

		/* Does service depends on current ?? */
		if (initng_depend(currentA, service) == FALSE)
			continue;

		switch (GET_STATE(currentA)) {
		/* if its done, this is perfect */
		case IS_DOWN:
			continue;

		/* If the dep is failed, continue */
		case IS_FAILED:
			continue;

		/* BIG TODO.
		 * This is not correct, but if we wait for a service that is
		 * starting to stop, and that service is waiting for this
		 * service to start, until it starts, makes a deadlock.
		 *
		 * Asuming that STARTING services WAITING_FOR_START_DEP are
		 * down for now.
		 */
		case IS_STARTING:
			if (strstr(currentA->current_state->name,
				   "WAITING_FOR_START_DEP"))
				continue;
			break;

		default:
			break;
		}

#ifdef DEBUG
		/* else RETURN */
		if (verbose)
			D_("still waiting for service %s state %s\n",
			   currentA->name, currentA->current_state->name);
		else
			D_("still waiting for service %s state %s\n",
			   currentA->name, currentA->current_state->name);
#endif

		/* no, the dependency are not met YET */
		service->depend_cache--;
		return FALSE;
	}

	/* run the global module dep check */
	{
		s_event event;

		event.event_type = &EVENT_STOP_DEP_MET;
		event.data = service;

		initng_event_send(&event);
		if (event.status == FAILED) {
			if (verbose) {
				F_("Service %s cannot be started because a "
				   "module (START_DEP_MET) says so.\n",
				   service->name);
			}

			return FALSE;
		}
	}

	service->depend_cache = 0;
	return TRUE;
}
