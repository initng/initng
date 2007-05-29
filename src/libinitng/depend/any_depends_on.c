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


/* check if any service in list, that is starting, running, or stopping
 * is depending on service
 * returns TRUE if any service depends * service
 */
int initng_depend_any_depends_on(active_db_h * service)
{
	active_db_h *current = NULL;

	D_("initng_any_depends_on(%s);\n", service->name);

	while_active_db(current)
	{
		/* Dont mind stop itself */
		if (current == service)
			continue;

		if (IS_UP(current) || IS_STARTING(current) || IS_STOPPING(current))
		{

			/* if current depends on service */
			if (initng_depend_deep(current, service) == TRUE)
			{
				D_("Service %s depends on %s\n", current->name,
				   service->name);
				return (TRUE);
			}
		}
	}
	D_("None found depending on %s.\n", service->name);
	return (FALSE);
}
