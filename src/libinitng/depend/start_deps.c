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
 * Start all deps, required or needed.
 * If a required deps failed to start, this will return FALSE
 */
int initng_depend_start_deps(active_db_h * service)
{
	active_db_h *dep = NULL;
	s_data *current = NULL;

	assert(service);
	assert(service->name);
#ifdef DEBUG
	D_("initng_depend_start_deps(%s);\n", service->name);
#endif

	/* walk all possible entrys, use get_next with NULL becouse we want both REQUIRE and NEED */
	while ((current = get_next(NULL, service, current)))
	{
		/* only intreseted in two types */
		if (current->type != &REQUIRE && current->type != &NEED)
			continue;

		/* to be sure */
		if (!current->t.s)
			continue;

		/* tell the user what we got */
		D_(" %s %s %s\n", service->name,
		   current->type == &REQUIRE ? "requires" : "needs", current->t.s);

		/* look if it exits already */
		if ((dep = initng_active_db_find_by_exact_name(current->t.s)))
		{
			D_("No need to LOAD \"%s\" == \"%s\", state %s it is already loaded!\n", current->t.s, dep->name, dep->current_state->state_name);
			/* start the service if its down */
			if (IS_DOWN(dep))
			{
				D_("Service %s is down, starting.\n", dep->name);
				initng_handler_start_service(dep);
			}
			continue;
		}

		D_("Starting new_service becouse not found: %s\n", current->t.s);
		/* if we where not succeded to start this new one */
		if (!initng_handler_start_new_service_named(current->t.s))
		{
			/* if its NEED */
			if (current->type == &NEED)
			{
				D_("service \"%s\" needs service \"%s\", that could not be found!\n", service->name, current->t.s);
				continue;
			}
			/* else its REQUIRE */
			else
			{
				F_("%s required dep \"%s\" could not start!\n", service->name,
				   current->t.s);
				initng_common_mark_service(service, &REQ_NOT_FOUND);
				return (FALSE);
			}
		}
		/* continue ; */
	}

	D_("initng_depend_start_deps(%s): DONE-TRUE\n", service->name);

	/* if we got here, its a sucess */
	return (TRUE);
}
