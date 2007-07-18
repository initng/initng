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
#include <stdlib.h>					/* free() exit() */
#include <string.h>
#include <assert.h>


/*
 * This will check with plug-ins if dependencies for start this is met.
 * If this returns FALSE deps are not met yet, try later.
 * If this returns FAIL deps wont ever be met, so stop trying.
 */
int initng_depend_start_dep_met(active_db_h *service, int verbose)
{
	active_db_h *dep = NULL;
	s_data *current = NULL;
	int count = 0;

	assert(service);
	assert(service->name);

	/* walk all possible entrys, use get_next with NULL because we want
	 * REQUIRE, NEED and USE */
	while ((current = get_next(NULL, service, current))) {
		/* only intreseted in two types */
		if (current->type != &REQUIRE && current->type != &NEED &&
		    current->type != &USE)
			continue;

		/* to be sure */
		if (!current->t.s)
			continue;

		/* this is a cache, that meens that calling this function over
		 * and over again, we dont make two checks on same entry */
		count++;
		if (service->depend_cache >= count) {
			D_("Dep %s is ok allredy for %s.\n", current->t.s,
			   service->name);
			continue;
		}


		/* tell the user what we got */
#ifdef DEBUG
		if (current->type == &REQUIRE)
			D_(" %s requires %s\n", service->name, current->t.s);
		else if (current->type == &NEED)
			D_(" %s needs %s\n", service->name, current->t.s);
		else if (current->type == &USE)
			D_(" %s uses %s\n", service->name, current->t.s);
#endif

		/* look if it exits already */
		if (!(dep = initng_active_db_find_by_exact_name(current->t.s))) {
			if (current->type == &USE) {
				/* if its not yet found, and i dont care */
				continue;
			}
			else if (current->type == &REQUIRE) {
				F_("%s required dep \"%s\" could not start!\n",
				   service->name, current->t.s);
				initng_common_mark_service(service, &REQ_NOT_FOUND);
				/* if its not yet found, this dep is not reached */
				return FAIL;
			} else { /* NEED */
				/* if its not yet found, this dep is not
				 * reached */
				return FALSE;
			}
		}

		/* if service dep on is starting, wait a bit */
		if (IS_STARTING(dep)) {
			if (verbose) {
				F_("Could not start service %s because it "
				   "depends on service %s that is still "
				   "starting.\n", service->name, dep->name);
			}
			return FALSE;
		}

		/* if service failed, return that */
		if (IS_FAILED(dep)) {
			if (verbose) {
				F_("Could not start service %s because it "
				   "depends on service %s that is failed.\n",
				   service->name, dep->name);
			}
			return FAIL;
		}

		/* if its this fresh, we dont do anything */
		if (IS_NEW(dep))
			return FALSE;

		/* if its marked down, and not starting, start it */
		if (IS_DOWN(dep)) {
		   initng_handler_start_service(dep);
		   return FALSE;
		}

		/* if its not starting or up, return FAIL */
		if (!IS_UP(dep)) {
			F_("Could not start service %s because it depends on "
			   "service %s has state %s\n", service->name,
			   dep->name, dep->current_state->name);
			return FALSE;
		}

		/* GOT HERE MEENS THAT ITS OK */
		service->depend_cache = count;
		D_("Dep %s is ok for %s.\n", current->t.s, service->name);
		/* continue; */
	}

	/* run the global plugin dep check */
	{
		s_event event;

		event.event_type = &EVENT_START_DEP_MET;
		event.data = service;

		initng_event_send(&event);
		if (event.status == FAILED) {
			if (verbose) {
				F_("Service %s can not be started because a "
				   "plugin (EVENT_START_DEP_MET) says so.\n",
				   service->name);
			}

			return FALSE;
		}
	}

	D_("dep met for %s\n", service->name);

	/* reset the count cache */
	service->depend_cache = 0;
	return TRUE;
}
