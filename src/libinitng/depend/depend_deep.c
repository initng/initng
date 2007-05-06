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
 * A deeper deep-find.
 * Logic, we wanna make sure that depend check in a deeper level.
 * if daemon/smbd -> daemon/samba -> system/checkroot -> system/initial.
 * So should initng_depend_deep(daemon/smbd, system/initial) == TRUE
 *
 * Summary, does service depends on check?
 */
static int initng_depend_deep2(active_db_h * service, active_db_h * check,
							   int *stack);
int initng_depend_deep(active_db_h * service, active_db_h * check)
{
	int stack = 0;

	return (initng_depend_deep2(service, check, &stack));
}

static int initng_depend_deep2(active_db_h * service, active_db_h * check,
							   int *stack)
{
	active_db_h *current = NULL;
	int result = FALSE;

	/* avoid cirular */
	if (current == service)
		return (FALSE);

	/* if service depends on check, it also dep_on_deep's on check */
	/* this serves as an exit from the recursion */
	if (initng_depend(service, check))
		return (TRUE);

	/* in case there is a circular dependency, break after 10 levels of recursion */
	(*stack)++;
	if (*stack > 10)
		return (FALSE);

	/* loop over all services, if service depends on current, recursively check if
	 * current may depend (deep) on check */
	while_active_db(current)
	{
		if (initng_depend(service, current))
		{
			if ((result = initng_depend_deep2(current, check, stack)))
				break;
		}
	}

	/* return with the result found at last */
	return (result);
}
