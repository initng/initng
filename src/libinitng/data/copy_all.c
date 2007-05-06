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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <initng.h>
#include "local.h"


/*
 * Walk the db, copy all strings in list from to to.
 */
void d_copy_all(data_head * from, data_head * to)
{
	s_data *tmp = NULL;
	s_data *current = NULL;

	list_for_each_entry(current, &from->head.list, list)
	{
		/* make sure type is set
		   TODO, should this be an assert(current->type) ??? */
		if (!current->type)
			continue;

		/* allocate the new one */
		tmp = (s_data *) i_calloc(1, sizeof(s_data));

		/* copy */
		memcpy(tmp, current, sizeof(s_data));

		/* copy the data */
		switch (current->type->opt_type)
		{
			case STRING:
			case STRINGS:
			case VARIABLE_STRING:
			case VARIABLE_STRINGS:
				if (current->t.s)
					tmp->t.s = i_strdup(current->t.s);
				break;
			default:
				break;
		}

		/* copy variable name */
		if (current->vn)
			tmp->vn = i_strdup(current->vn);
		else
			tmp->vn = NULL;

		/* add to list */
		list_add(&tmp->list, &to->head.list);
	}
}
