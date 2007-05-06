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

#include <stdio.h>
#include <stdlib.h>							/* free() exit() */
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

	/*
	 *    Check so all deps, that needs service, is down.
	 *    if there are services depending on this one still running, return false and still try
	 */
	while_active_db(currentA)
	{
		count++;
		if (service->depend_cache >= count)
			continue;

		/* temporary increase depend_cache - will degrese before reutrn (FALSE) */
		service->depend_cache++;

		if (currentA == service)
			continue;

		/* Does service depends on current ?? */
		if (initng_depend(currentA, service) == FALSE)
			continue;

		/* if its done, this is perfect */
		if (IS_DOWN(currentA))
			continue;

		/* If the dep is failed, continue */
		if (IS_FAILED(currentA))
			continue;

		/* BIG TODO.
		 * This is not correct, but if we wait for a service that is
		 * starting to stop, and that service is waiting for this service
		 * to start, until it starts, makes a deadlock.
		 *
		 * Asuming that STARTING services WAITING_FOR_START_DEP are down for now.
		 */
		if (IS_STARTING(currentA)
			&& strstr(currentA->current_state->state_name,
					  "WAITING_FOR_START_DEP"))
			continue;

#ifdef DEBUG
		/* else RETURN */
		if (verbose)
			D_("still waiting for service %s state %s\n", currentA->name,
			   currentA->current_state->state_name);
		else
			D_("still waiting for service %s state %s\n", currentA->name,
			   currentA->current_state->state_name);
#endif

		/* if its still marked as UP and not stopping, tell the service AGAIN nice to stop */
		/*if (IS_UP(currentA))
		   initng_handler_stop_service(currentA); */

		/* no, the dependency are not met YET */
		service->depend_cache--;
		return (FALSE);
	}


	/* run the global plugin dep check */
	{
		s_event event;

		event.event_type = &EVENT_STOP_DEP_MET;
		event.data = service;

		initng_event_send(&event);
		if (event.status == FAILED)
		{
			if (verbose)
			{
				F_("Service %s can not be started because a plugin (START_DEP_MET) says so.\n", service->name);
			}

			return (FALSE);
		}
	}

	service->depend_cache = 0;
	return (TRUE);
}
