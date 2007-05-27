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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "local.h"

/*
 * This function will return a s_data pointer, matching type, vn
 * by the adress struct list.
 */
s_data *initng_data_get_next_var(s_entry * type, const char *vn,
                                 data_head * head, s_data * last)
{
	assert(head);
	struct list_head *place = NULL;
	s_data *current = NULL;

	ALIAS_WALK;

	/*
	 * There is a problem, functions run get_next_var, and want all variables, not just
	 * the onse with a special vn, so searching for variables with an vn, shud be possible
	 * without setting an vn to search for
	 */
#ifdef HURT_ME
	/* Check that vn is sent, when needed */
	if (!vn && type && type->opt_type >= 50)
	{
		F_("The vn variable is missing for a type %i (%s): %s!\n",
		   type->opt_type, type->opt_name);
		return (NULL);
	}

	/* check that vn is not set, when not needed */
	if (vn && type && type->opt_type < 50)
	{
		F_("The vn %s is set, but not needed for type %i, %s\n", vn,
		   type->opt_type, type->opt_name);
		return (NULL);
	}
#endif

	/* Now do the first hook if set */
	if (head->data_request && (*head->data_request) (head) == FALSE)
		return (NULL);

	/* Make sure the list is not empty */
	if (!list_empty(&head->head.list))
		/* put place on the initial */
		place = head->head.list.prev;

	/* as long as we dont stand on the header */
	while (place && place != &head->head.list)
	{
		/* get the entry */
		current = list_entry(place, s_data, list);

		/* if last still set, fast forward */
		if (last && current != last)
		{
			place = place->prev;
			continue;
		}

		/* if this is the last entry */
		if (last == current)
		{
			last = NULL;
			place = place->prev;
			continue;
		}

		/* Make sure the string variable name matches if set */
		if ((!type || current->type == type)
			&& (!current->vn || !vn || strcasecmp(current->vn, vn) == 0))
		{
			return (current);
		}

		/* try next */
		place = place->prev;
	}


	/* This second hook might fill head->res */
	if (head->res_request && (*head->res_request) (head) == FALSE)
		return (NULL);

	/* if there is any resursive next to check, do that. */
	if (head->res)
	{
		/* ok return with that */
		return (initng_data_get_next_var(type, vn, head->res, last));
	}

	/* no luck */
	return (NULL);
}
