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

#include "local.h"


/*
 * initng_depend:
 *  Will check with all plug-ins and return TRUE
 *  if service depends on check.
 */
int initng_depend(active_db_h * service, active_db_h * check)
{
	assert(service);
	assert(check);

	/* it can never depend on itself */
	if (service == check)
		return (FALSE);

	/* run the local static dep check */
	if (dep_on(service, check) == TRUE)
		return (TRUE);

	/* run the global plugin dep check */
	{
		s_event event;
		s_event_dep_on_data data;

		event.event_type = &EVENT_DEP_ON;
		event.data = &data;
		data.service = service;
		data.check = check;

		initng_event_send(&event);
		if (event.status == OK)
			return (TRUE);
	}

	/* No, "service" was not depending on "check" */
	return (FALSE);
}
