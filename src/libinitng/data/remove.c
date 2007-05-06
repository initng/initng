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
 * A function to nicely free the s_data content.
 */
static void d_dfree(s_data * current)
{
	assert(current);
	assert(current->type);

	/* Unlink this entry from any list */
	list_del(&current->list);

	/* free variable data */
	switch (current->type->opt_type)
	{
		case STRING:
		case STRINGS:
		case VARIABLE_STRING:
		case VARIABLE_STRINGS:
			if (current->t.s)
				free(current->t.s);
			current->t.s = NULL;
			break;
		default:
			break;
	}

	/* free variable name, if set */
	if (current->vn)
		free(current->vn);

	current->vn = NULL;

	/* ok, free the struct */
	free(current);
}

void d_remove_all(data_head * d)
{
	s_data *current = NULL;
	s_data *s = NULL;

	assert(d);

	/* walk through all entries on address */
	list_for_each_entry_safe(current, s, &d->head.list, list)
	{
		/* walk, and remove all */
		d_dfree(current);
		current = NULL;
	}

	/* make sure its cleared */
	DATA_HEAD_INIT(d);
}

void d_remove_var(s_entry * type, const char *vn, data_head * d)
{
	s_data *current;

	assert(d);
	assert(type);

	if (!type)
	{
		F_("Type can't be zero!\n");
		return;
	}

	/* for every matching, free it */
	while ((current = d_get_next_var(type, vn, d, NULL)))
	{
		d_dfree(current);
	}
}
